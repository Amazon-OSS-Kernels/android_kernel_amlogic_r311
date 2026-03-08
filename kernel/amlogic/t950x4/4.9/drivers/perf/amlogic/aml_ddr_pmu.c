// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>

#include <linux/bitfield.h>
#include <linux/init.h>
#include <linux/perf_event.h>

#include <asm-generic/io.h>

#include <linux/amlogic/cpu_version.h>
#include <soc/amlogic/aml_ddr_pmu.h>

#define DDR_PERF_DEV_NAME "aml_ddrc"

#define to_ddr_pmu(p)		container_of(p, struct ddr_pmu, pmu)

#define hw_info_to_pmu(p)	container_of(p, struct ddr_pmu, info)

struct ddr_pmu {
	struct pmu pmu;
	struct dmc_hw_info info;
	struct ddr_grant_info counters;
	bool pmu_enabled;
	spinlock_t lock; /* Protect the hw counter and sw counter */
	struct device *dev;
	char *name;
	struct perf_event *events[COUNTER_MAX_ID];
	int event_num;
	u8 idx_array[COUNTER_MAX_ID];
	struct	hlist_node node;
	enum cpuhp_state cpuhp_state;
	int sp;
	int cpu;
	int id;
};

static DEFINE_IDA(ddr_ida);

static void dmc_pmu_enable(struct ddr_pmu *pmu);
static void dmc_pmu_disable(struct ddr_pmu *pmu);

static void events_alloc_idx_init(struct ddr_pmu *pmu)
{
	int i;

	for (i = 0; i < COUNTER_MAX_ID; i++)
		pmu->idx_array[i] = COUNTER_MAX_ID - i - 1;

	pmu->sp = i - 1;
}

static inline int events_alloc_idx(struct ddr_pmu *pmu)
{
	int idx;

	WARN_ON(pmu->sp < 0);

	idx = pmu->idx_array[pmu->sp--];

	return idx;
}

static inline void events_free_idx(struct ddr_pmu *pmu, int idx)
{
	pmu->idx_array[++pmu->sp] = idx;

	WARN_ON(pmu->sp >= COUNTER_MAX_ID);
}

static inline void ddr_grant_accumulate(struct ddr_pmu *pmu,
					struct ddr_grant_info *c)
{
	struct ddr_grant_info *cnter = &pmu->counters;
	int chann_nr = pmu->info.chann_nr;
	int i;

	cnter->all_grant += c->all_grant;
	cnter->all_req += c->all_req;
	for (i = 0; i < chann_nr; i++)
		cnter->channel_grant[i] += c->channel_grant[i];
}

static inline void ddr_grant_add(struct ddr_grant_info *sum,
		struct ddr_grant_info *add1, struct ddr_grant_info *add2)
{
	int i;
	u64 grant1, grant2;

	sum->all_grant = add1->all_grant + add2->all_grant;
	sum->all_req = add1->all_req + add2->all_req;
	for (i = 0; i < MAX_CHANNEL; i++) {
		grant1 = add1->channel_grant[i];
		grant2 = add2->channel_grant[i];

		sum->channel_grant[i] = grant1 + grant2;
	}
}

static void aml_ddr_perf_event_update(struct perf_event *event)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	u64 new_raw_count = 0;
	struct ddr_grant_info dg = {0}, sum_dg = {0};
	int idx;

	spin_lock(&pmu->lock);
	// get the remain counters in register.
	pmu->info.ops->get_counters(&pmu->info, &dg);

	ddr_grant_add(&sum_dg, &pmu->counters, &dg);
	spin_unlock(&pmu->lock);

	switch (event->attr.config) {
	case CYCLE_COUNTER_ID:
		new_raw_count = sum_dg.all_req;
		break;
	case ALL_CHAN_COUNTER_ID:
		new_raw_count = sum_dg.all_grant;
		break;
	case CHAN1_COUNTER_ID:
	case CHAN2_COUNTER_ID:
	case CHAN3_COUNTER_ID:
	case CHAN4_COUNTER_ID:
	case CHAN5_COUNTER_ID:
	case CHAN6_COUNTER_ID:
	case CHAN7_COUNTER_ID:
	case CHAN8_COUNTER_ID:
		idx = event->attr.config - CHAN1_COUNTER_ID;
		new_raw_count = sum_dg.channel_grant[idx];
		break;
	case ALL_CHAN_IDLE_CNT_ID:
		new_raw_count = sum_dg.all_chann_idle;
		break;
	default:
		pr_err("unsupported counter id\n ");
	}

	local64_set(&event->count, new_raw_count);
}

static int aml_ddr_perf_event_init(struct perf_event *event)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	struct perf_event *sibling;

	if (event->attr.type != event->pmu->type)
		return -ENOENT;

	if (is_sampling_event(event) || event->attach_state & PERF_ATTACH_TASK)
		return -EOPNOTSUPP;

	if (event->cpu < 0) {
		dev_warn(pmu->dev, "Can't provide per-task data!\n");
		return -EOPNOTSUPP;
	}

	if (event->group_leader->pmu != event->pmu &&
	    !is_software_event(event->group_leader))
		return -EINVAL;

	if ((event)->group_leader == (event))
		list_for_each_entry((sibling), &(event)->sibling_list,
				sibling_list) {
			if (sibling->pmu != event->pmu &&
					!is_software_event(sibling))
				return -EINVAL;
		}

	//The SoC doesn't support this event
	if (pmu->info.event_flag[event->attr.config] == 0)
		return -EINVAL;

	event->cpu = pmu->cpu;
	hwc->idx = -1;

	return 0;
}

static void aml_ddr_perf_event_start(struct perf_event *event, int flags)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	local64_set(&hwc->prev_count, 0);

	local64_set(&event->count, 0);

	hwc->state = 0;
	memset(&pmu->counters, 0, sizeof(pmu->counters));

	dmc_pmu_enable(pmu);
}

static void aml_ddr_config_port(struct ddr_pmu *pmu, int port, int chann)
{
	if (pmu->info.ops->config_port)
		pmu->info.ops->config_port(&pmu->info, port, chann);
}

static void aml_ddr_set_filter(struct perf_event *event, int port)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	int chann;

	if (event->attr.config > ALL_CHAN_COUNTER_ID &&
		event->attr.config < COUNTER_MAX_ID) {
		chann = event->attr.config - CHAN1_COUNTER_ID;
		aml_ddr_config_port(pmu, port, chann);
	}
}

static int aml_ddr_perf_event_add(struct perf_event *event, int flags)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	u32 cfg1 = event->attr.config1;

	hwc->idx = events_alloc_idx(pmu);

	hwc->state |= PERF_HES_STOPPED;

	pmu->events[hwc->idx] = event;

	pmu->event_num++;

	aml_ddr_set_filter(event, cfg1);

	if (flags & PERF_EF_START)
		aml_ddr_perf_event_start(event, flags);

	return 0;
}

static void aml_ddr_perf_event_stop(struct perf_event *event, int flags)
{
	struct hw_perf_event *hwc = &event->hw;
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);

	if (flags & PERF_EF_UPDATE)
		aml_ddr_perf_event_update(event);

	dmc_pmu_disable(pmu);
	hwc->state |= PERF_HES_STOPPED;
}

static void aml_ddr_perf_event_del(struct perf_event *event, int flags)
{
	struct ddr_pmu *pmu = to_ddr_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	aml_ddr_perf_event_stop(event, PERF_EF_UPDATE);

	pmu->events[hwc->idx] = NULL;

	events_free_idx(pmu, hwc->idx);
	pmu->event_num--;
	hwc->idx = -1;
}

static ssize_t aml_ddr_perf_cpumask_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct ddr_pmu *pmu = dev_get_drvdata(dev);

	return cpumap_print_to_pagebuf(true, buf, cpumask_of(pmu->cpu));
}

static struct device_attribute aml_ddr_perf_cpumask_attr =
__ATTR(cpumask, 0444, aml_ddr_perf_cpumask_show, NULL);

static struct attribute *aml_ddr_perf_cpumask_attrs[] = {
	&aml_ddr_perf_cpumask_attr.attr,
	NULL,
};

static const struct attribute_group ddr_perf_cpumask_attr_group = {
	.attrs = aml_ddr_perf_cpumask_attrs,
};

static ssize_t
ddr_pmu_event_show(struct device *dev, struct device_attribute *attr,
		   char *page)
{
	struct perf_pmu_events_attr *pmu_attr;

	pmu_attr = container_of(attr, struct perf_pmu_events_attr, attr);
	return sprintf(page, "event=0x%02llx\n", pmu_attr->id);
}

#define AML_DDR_PMU_EVENT_ATTR(_name, _id)			\
{								\
	.attr = __ATTR(_name, 0444, ddr_pmu_event_show, NULL),	\
	.id = _id,						\
}

static struct perf_pmu_events_attr event_attrs[] = {
	AML_DDR_PMU_EVENT_ATTR(all_req_cycles, CYCLE_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_all_read_write_cycles, ALL_CHAN_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_1_read_write_cycles, CHAN1_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_2_read_write_cycles, CHAN2_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_3_read_write_cycles, CHAN3_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_4_read_write_cycles, CHAN4_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_5_read_write_cycles, CHAN5_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_6_read_write_cycles, CHAN6_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_7_read_write_cycles, CHAN7_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(chan_8_read_write_cycles, CHAN8_COUNTER_ID),
	AML_DDR_PMU_EVENT_ATTR(all_channel_idle_cycles, ALL_CHAN_IDLE_CNT_ID),
};

static struct attribute *ddr_perf_events_attrs[COUNTER_MAX_ID];

static struct attribute_group ddr_perf_events_attr_group = {
	.name = "events",
	.attrs = ddr_perf_events_attrs,
};

PMU_FORMAT_ATTR(event, "config:0-7");
PMU_FORMAT_ATTR(axi_id, "config1:0-7");

static struct attribute *ddr_perf_format_attrs[] = {
	&format_attr_event.attr,
	&format_attr_axi_id.attr,
	NULL,
};

static struct attribute_group ddr_perf_format_attr_group = {
	.name = "format",
	.attrs = ddr_perf_format_attrs,
};

static ssize_t aml_ddr_perf_identifier_show(struct device *dev,
		struct device_attribute *attr,
		char *page)
{
	struct ddr_pmu *pmu = dev_get_drvdata(dev);

	return sprintf(page, "%s\n", pmu->info.name);
}

static umode_t aml_ddr_perf_identifier_attr_visible(struct kobject *kobj,
		struct attribute *attr,
		int n)
{
	return attr->mode;
}

static struct device_attribute aml_ddr_perf_identifier_attr =
__ATTR(identifier, 0444, aml_ddr_perf_identifier_show, NULL);

static struct attribute *aml_ddr_perf_identifier_attrs[] = {
	&aml_ddr_perf_identifier_attr.attr,
	NULL,
};

static const struct attribute_group ddr_perf_identifier_attr_group = {
	.attrs = aml_ddr_perf_identifier_attrs,
	.is_visible = aml_ddr_perf_identifier_attr_visible,
};

static const struct attribute_group *attr_groups[] = {
	&ddr_perf_events_attr_group,
	&ddr_perf_format_attr_group,
	&ddr_perf_cpumask_attr_group,
	&ddr_perf_identifier_attr_group,
	NULL,
};

static inline void dmc_pmu_enable(struct ddr_pmu *pmu)
{
	if (!pmu->pmu_enabled && pmu->info.ops->enable)
		pmu->info.ops->enable(&pmu->info);

	pmu->pmu_enabled = true;
}

static inline void dmc_pmu_disable(struct ddr_pmu *pmu)
{
	pmu->pmu_enabled = false;
}

static irqreturn_t dmc_irq_handler(int irq, void *dev_id)
{
	struct dmc_hw_info *info = dev_id;
	struct ddr_pmu *pmu;
	struct ddr_grant_info counters;

	if (irq != info->irq_num)
		return IRQ_HANDLED;

	pmu = hw_info_to_pmu(info);
	WARN_ON(!info->ops->irq_handler);

	/* compatible with ddr bandwidth driver
	 * check if the irq is enabled by ddr
	 * bandwidth driver
	 */
	if (info->ops->irq_identify &&
	    !info->ops->irq_identify(info))
		return IRQ_HANDLED;

	if (pmu->pmu_enabled) {
		spin_lock(&pmu->lock);
		if (info->ops->irq_handler(info, &counters) != 0) {
			spin_unlock(&pmu->lock);
			goto out;
		}

		ddr_grant_accumulate(pmu, &counters);
		spin_unlock(&pmu->lock);
		/*
		 * the timer interrupt only supprt
		 * single mode, we have to re-enable
		 * it in ISR to support continue mode.
		 */
		info->ops->enable(info);
	} else {
		info->ops->disable(info);
	}

	pr_debug("counts: %llu %llu %llu, %llu, %llu, %llu\t\t"
			"sum: %llu %llu %llu, %llu, %llu, %llu\n",
			counters.all_req,
			counters.all_grant,
			counters.channel_grant[0],
			counters.channel_grant[1],
			counters.channel_grant[2],
			counters.channel_grant[3],

			pmu->counters.all_req,
			pmu->counters.all_grant,
			pmu->counters.channel_grant[0],
			pmu->counters.channel_grant[1],
			pmu->counters.channel_grant[2],
			pmu->counters.channel_grant[3]);
out:
	return IRQ_HANDLED;
}

static int ddr_perf_offline_cpu(unsigned int cpu, struct hlist_node *node)
{
	struct ddr_pmu *pmu = hlist_entry_safe(node, struct ddr_pmu, node);
	int target;

	if (cpu != pmu->cpu)
		return 0;

	target = cpumask_any_but(cpu_online_mask, cpu);
	if (target >= nr_cpu_ids)
		return 0;

	perf_pmu_migrate_context(&pmu->pmu, cpu, target);
	pmu->cpu = target;

	WARN_ON(irq_set_affinity_hint(pmu->info.irq_num, cpumask_of(pmu->cpu)));

	return 0;
}

static int ddr_pmu_get_hw_info(struct platform_device *pdev,
				struct dmc_hw_info *info)
{
	int ret = -EINVAL;
#ifdef CONFIG_OF
	struct device_node *node = pdev->dev.of_node;
	/*struct pinctrl *p;*/
	struct resource *res;
	resource_size_t *base;
	const char *irq_name;
	u32 dmc_nr = 1;
	int i;

	if (of_property_read_u32(node, "dmc_nr", &dmc_nr))
		dmc_nr = 1;

	info->dmc_nr = dmc_nr;

	for (i = 0; i < dmc_nr; i++) {
		/* resource 0 for ddr register base */
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(&pdev->dev, "can't get ddr reg base\n");
			return -EINVAL;
		}
		base = ioremap(res->start, res->end - res->start);
		if (!base) {
			dev_err(&pdev->dev, "couldn't ioremap ddr reg\n");
			return -EINVAL;
		}
		info->ddr_reg[i] = (void *)base;
	}

	/* resource i for pll register base */
	res = platform_get_resource(pdev, IORESOURCE_MEM, i);
	if (!res) {
		dev_err(&pdev->dev, "can't get ddr reg base\n");
		goto err1;
	}
	base = ioremap(res->start, res->end - res->start);
	if (!base) {
		dev_err(&pdev->dev, "couldn't ioremap for pll reg\n");
		goto err1;
	}
	info->pll_reg = (void *)base;

	info->irq_num = of_irq_get(node, 0);
	if (info->irq_num < 0) {
		dev_err(&pdev->dev, "couldn't get irq\n");
		goto err2;
	}

	irq_name = of_get_property(node, "interrupt-names", NULL);
	if (!irq_name)
		irq_name = "ddr_pmu";

	ret = request_irq(info->irq_num, dmc_irq_handler,
				IRQF_SHARED, irq_name, (void *)info);
	if (ret < 0) {
		dev_err(&pdev->dev, "ddr request irq failed\n");
		goto err2;
	}

	info->ops = t5_dmc_pmu_init(info);
	if (!IS_ERR(info->ops)) {
		dev_info(&pdev->dev, "find the right version of dmc t5d\n");
		return 0;
	}

	/*************error path *********************/
	ret = PTR_ERR(info->ops);
	free_irq(info->irq_num, info);

err2:
	iounmap(info->pll_reg);
err1:
	for (i = 0; i < info->dmc_nr; i++)
		iounmap(info->ddr_reg[i]);
#endif
	return ret;
}

static void fill_event_attr(struct ddr_pmu *pmu)
{
	int i, j, k;
	struct attribute **dst = ddr_perf_events_attrs;
	u8 *event_flag = pmu->info.event_flag;

	/*
	 * firstly, find the supported event from pmu info
	 * secondly, find current id equal to the temp
	 * finally, assignment
	 */
	for (i = 0, j = 0; i < COUNTER_MAX_ID; i++) {
		if (event_flag[i] != EVENT_SUPPORT)
			continue;

		for (k = 0; k < COUNTER_MAX_ID; k++) {
			if (i == event_attrs[k].id) {
				dst[j++] = &event_attrs[k].attr.attr;
				break;
			}
		}
	}

	dst[j] = NULL; /* mark end */
}

static int __init ddr_pmu_probe(struct platform_device *pdev)
{
	int ret, i;
	char *name;
	struct ddr_pmu *pmu = devm_kzalloc(&pdev->dev,
					   sizeof(struct ddr_pmu),
					   GFP_KERNEL);
	if (!pmu) {
		dev_err(&pdev->dev, "no memory\n");
		return -ENOMEM;
	}

	*pmu = (struct ddr_pmu) {
		.pmu = (struct pmu) {
			.module	     = THIS_MODULE,
//			.capabilities = PERF_PMU_CAP_NO_EXCLUDE,
			.task_ctx_nr = perf_invalid_context,
			.attr_groups = attr_groups,
			.event_init  = aml_ddr_perf_event_init,
			.add	     = aml_ddr_perf_event_add,
			.del	     = aml_ddr_perf_event_del,
			.start	     = aml_ddr_perf_event_start,
			.stop	     = aml_ddr_perf_event_stop,
			.read	     = aml_ddr_perf_event_update,
		}
	};

	spin_lock_init(&pmu->lock);

	if (ddr_pmu_get_hw_info(pdev, &pmu->info) < 0) {
		dev_err(&pdev->dev, "couldn't get hw info\n");
		ret = -EINVAL;
		goto get_hw_info_err;
	}

	pmu->id = ida_simple_get(&ddr_ida, 0, 0, GFP_KERNEL);
	if (pmu->id < 0) {
		dev_err(&pdev->dev, "get id failed %d\n", pmu->id);
		ret = pmu->id;
		goto ida_get_err;
	}

	pmu->cpu = raw_smp_processor_id();

	name = devm_kasprintf(&pdev->dev, GFP_KERNEL, DDR_PERF_DEV_NAME "%d", pmu->id);
	if (!name) {
		dev_err(&pdev->dev, "couldn't alloc memory for name\n");
		ret = -ENOMEM;
		goto get_dev_name;
	}

	ret = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
			name,
			NULL,
			ddr_perf_offline_cpu);

	if (ret < 0) {
		dev_err(&pdev->dev, "cpuhp_setup_state_multi failed\n");
		goto cpuhp_state_err;
	}
	pmu->cpuhp_state = ret;

	/* Register the pmu instance for cpu hotplug */
	ret = cpuhp_state_add_instance_nocalls(pmu->cpuhp_state, &pmu->node);
	if (ret) {
		dev_err(&pdev->dev, "Error %d registering hotplug\n", ret);
		goto cpuhp_instance_err;
	}

	fill_event_attr(pmu);

	ret = perf_pmu_register(&pmu->pmu, name, -1);
	if (ret) {
		dev_err(&pdev->dev, "perf pmu register failed\n");
		goto pmu_register_err;
	}

	pmu->name = name;
	pmu->event_num = 0;
	pmu->dev = &pdev->dev;
	pmu->pmu_enabled = false;

	for (i = 0; i < COUNTER_MAX_ID; i++)
		pmu->events[i] = NULL;

	events_alloc_idx_init(pmu);

	platform_set_drvdata(pdev, pmu);

	dev_info(&pdev->dev, "ddr perf init ok\n");

	return 0;

pmu_register_err:
	cpuhp_state_remove_instance_nocalls(pmu->cpuhp_state, &pmu->node);
cpuhp_instance_err:
	cpuhp_remove_multi_state(pmu->cpuhp_state);
cpuhp_state_err:
	devm_kfree(&pdev->dev, name);
get_dev_name:
	ida_simple_remove(&ddr_ida, pmu->id);
ida_get_err:
	free_irq(pmu->info.irq_num, &pmu->info);
	for (i = 0; i < pmu->info.dmc_nr; i++)
		iounmap(pmu->info.ddr_reg[i]);
	iounmap(pmu->info.pll_reg);
get_hw_info_err:
	devm_kfree(&pdev->dev, pmu);

	return ret;
}

static int ddr_pmu_remove(struct platform_device *pdev)
{
	struct ddr_pmu *pmu = platform_get_drvdata(pdev);
	int i;

	perf_pmu_unregister(&pmu->pmu);
	cpuhp_state_remove_instance_nocalls(pmu->cpuhp_state, &pmu->node);
	cpuhp_remove_multi_state(pmu->cpuhp_state);
	devm_kfree(pmu->dev, pmu->name);
	free_irq(pmu->info.irq_num, &pmu->info);
	for (i = 0; i < pmu->info.dmc_nr; i++)
		iounmap(pmu->info.ddr_reg[i]);
	iounmap(pmu->info.pll_reg);
	ida_simple_remove(&ddr_ida, pmu->id);

	dev_info(&pdev->dev, "ddr perf finit ok\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id aml_ddr_pmu_dt_match[] = {
	{
		.compatible = "amlogic,ddr-pmu",
	},
	{}
};
#endif

static struct platform_driver ddr_pmu_driver = {
	.driver = {
		.name = "amlogic-ddr-pmu",
		.owner = THIS_MODULE,
	#ifdef CONFIG_OF
		.of_match_table = aml_ddr_pmu_dt_match,
	#endif
	},
	.remove = ddr_pmu_remove,
};

static int __init aml_ddr_pmu_init(void)
{
	return platform_driver_probe(&ddr_pmu_driver, ddr_pmu_probe);
}

static void __exit aml_ddr_pmu_exit(void)
{
	platform_driver_unregister(&ddr_pmu_driver);
}

module_init(aml_ddr_pmu_init);
module_exit(aml_ddr_pmu_exit);
MODULE_LICENSE("GPL v2");

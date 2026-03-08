/*
 * drivers/amlogic/power/sec_power_domain.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/amlogic/power_domain.h>
#include <dt-bindings/power/t5-pd.h>

struct sec_pm_domain {
	struct generic_pm_domain base;
	int pd_index;
	bool pd_status;
};

struct sec_pm_domain_data {
	struct sec_pm_domain *domains;
	unsigned int domains_count;
};

static inline struct sec_pm_domain *
to_sec_pm_domain(struct generic_pm_domain *genpd)
{
	return container_of(genpd, struct sec_pm_domain, base);
}

static int sec_pm_domain_power_off(struct generic_pm_domain *genpd)
{
	struct sec_pm_domain *pd = to_sec_pm_domain(genpd);

	if (pd->base.flags == FLAG_ALWAYS_ON)
		return 0;

	pwr_ctrl_psci_smc(pd->pd_index, PWR_OFF);

	return 0;
}

static int sec_pm_domain_power_on(struct generic_pm_domain *genpd)
{
	struct sec_pm_domain *pd = to_sec_pm_domain(genpd);

	if (pd->base.flags == FLAG_ALWAYS_ON)
		return 0;

	pwr_ctrl_psci_smc(pd->pd_index, PWR_ON);

	return 0;
}

#define POWER_DOMAIN(_name, index, status, flag)		\
{					\
		.base = {					\
			.name = #_name,				\
			.power_off = sec_pm_domain_power_off,	\
			.power_on = sec_pm_domain_power_on,	\
			.flags = flag, \
		},						\
		.pd_index = index,				\
		.pd_status = status,				\
}

static struct sec_pm_domain t5_pm_domains[] = {
	[PDID_T5_DOS_HEVC] = POWER_DOMAIN(hevc, PDID_T5_DOS_HEVC, DOMAIN_INIT_OFF, 0),
	[PDID_T5_DOS_VDEC] = POWER_DOMAIN(vdec, PDID_T5_DOS_VDEC, DOMAIN_INIT_OFF, 0),
	[PDID_T5_VPU_HDMI] = POWER_DOMAIN(vpu, PDID_T5_VPU_HDMI, DOMAIN_INIT_ON, FLAG_ALWAYS_ON),
	[PDID_T5_DEMOD] = POWER_DOMAIN(demod, PDID_T5_DEMOD, DOMAIN_INIT_OFF, 0),
};

static struct sec_pm_domain_data t5_pm_domain_data = {
	.domains = t5_pm_domains,
	.domains_count = ARRAY_SIZE(t5_pm_domains),
};

static int sec_pd_probe(struct platform_device *pdev)
{
	int ret, i;
	int init_status;
	struct sec_pm_domain *pd;
	const struct sec_pm_domain_data *match;
	struct genpd_onecell_data *sec_pd_onecell_data;

	match = of_device_get_match_data(&pdev->dev);
	if (!match) {
		dev_err(&pdev->dev, "failed to get match data\n");
		return -ENODEV;
	}

	sec_pd_onecell_data = devm_kzalloc(&pdev->dev, sizeof(*sec_pd_onecell_data), GFP_KERNEL);
	if (!sec_pd_onecell_data)
		return -ENOMEM;

	sec_pd_onecell_data->domains = devm_kcalloc(&pdev->dev, match->domains_count,
						    sizeof(*sec_pd_onecell_data->domains),
						    GFP_KERNEL);
	if (!sec_pd_onecell_data->domains)
		return -ENOMEM;

	sec_pd_onecell_data->num_domains = match->domains_count;

	for (i = 0; i < match->domains_count; i++) {
		pd = &match->domains[i];

		/* array might be sparse */
		if (!pd->base.name)
			continue;

		/* Initialize based on pd_status */
		init_status = pwr_ctrl_status_psci_smc(pd->pd_index);
		if (init_status == -1)
			init_status = pd->pd_status;

		pm_genpd_init(&pd->base, NULL, init_status);
		sec_pd_onecell_data->domains[i] = &pd->base;
	}

	pd_dev_create_file(&pdev->dev, 0, sec_pd_onecell_data->num_domains,
			   sec_pd_onecell_data->domains);

	ret = of_genpd_add_provider_onecell(pdev->dev.of_node,
					    sec_pd_onecell_data);

	if (ret)
		goto out;

	return 0;

out:
	pd_dev_remove_file(&pdev->dev);
	return ret;
}

static const struct of_device_id pd_match_table[] = {
	{
		.compatible = "amlogic,t5-power-domain",
		.data = &t5_pm_domain_data,
	},
	{}
};

static struct platform_driver sec_pd_driver = {
	.probe		= sec_pd_probe,
	.driver		= {
		.name	= "sec_pd",
		.of_match_table = pd_match_table,
	},
};

static int sec_pd_init(void)
{
	return platform_driver_register(&sec_pd_driver);
}
arch_initcall_sync(sec_pd_init)

MODULE_AUTHOR("Amlogic, Inc.");
MODULE_DESCRIPTION("Amlogic Power domain driver");
MODULE_LICENSE("GPL");

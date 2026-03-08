obj-y	+=	amvideo.o
CFLAGS_video.o := -I$(src)
amvideo-objs += video.o video_keeper.o vpp.o video_hw.o vpp_pq_process.o video_receiver.o video_reg.o video_signal_notify.o
ifneq ($(CONFIG_ENABLE_AFD),)
amvideo-objs +=vpp_afd.o
endif
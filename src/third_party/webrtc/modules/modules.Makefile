# This file is generated by gyp; do not edit.

export builddir_name ?= src/third_party/webrtc/modules/out
.PHONY: all
all:
	$(MAKE) -C ../../.. paced_sender G711 G722 bitrate_controller iSACFix audio_processing_sse2 iLBC CNG NetEq webrtc_i420 iSAC video_processing_sse2 PCM16B remote_bitrate_estimator webrtc_opus audio_device rtp_rtcp audio_processing audio_coding_module webrtc_video_coding webrtc_utility audio_conference_mixer video_processing video_render_module media_file video_capture_module
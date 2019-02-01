# docker image for iccpd agent

DOCKER_ICCPD = docker-iccpd.gz
$(DOCKER_ICCPD)_PATH = $(DOCKERS_PATH)/docker-iccpd
$(DOCKER_ICCPD)_DEPENDS += $(SWSS) $(REDIS_TOOLS) $(ICCPD)
$(DOCKER_ICCPD)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE)
SONIC_DOCKER_IMAGES += $(DOCKER_ICCPD)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_ICCPD)

$(DOCKER_ICCPD)_CONTAINER_NAME = iccpd
$(DOCKER_ICCPD)_RUN_OPT += --net=host --privileged -t
$(DOCKER_ICCPD)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro

$(DOCKER_ICCPD)_BASE_IMAGE_FILES += mclagdctl:/usr/bin/mclagdctl

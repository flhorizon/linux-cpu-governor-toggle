
BIN := hold_perf_mode

# It could be 'sudo -u mydockergroupuser docker'
DOCKER ?= docker
DOCKER_COMPILER_IMAGE ?= gcc
# Override to, e.g. 'true;', if the current environment doesn't support 'sudo'
SUDO ?= sudo
# Destination
DESTINATION_DIR ?= $(PWD)

.PHONY = build chown setuid clean docker docker-build install



all: build install chown setuid

# A target running the build phase through docker.
docker: docker-build install chown setuid

$(BIN): $(BIN:=.c)
	cc -o $(BIN) $<


build: $(BIN)

# gcc image has `make'. Not sure the setuid bit will be properly set on host context.
docker-build:
	$(DOCKER) run --rm \
		-u $(shell id -u) \
		-v $(PWD):/tmp/workdir -w /tmp/workdir \
		$(DOCKER_COMPILER_IMAGE) \
		make build
	@echo 'setuid' target has to be run on the host context.

chown:
	$(SUDO) chown 0:0 "$(DESTINATION_DIR)/$(BIN)"

# The setuid bit cannot be set through docker volumes; is cleared when moving the file.
setuid:
	$(SUDO) chmod 4755 "$(DESTINATION_DIR)/$(BIN)"

install:
	if [ ! "$(PWD)" = "$(DESTINATION_DIR)" ];\
		then cp $(BIN) "$(DESTINATION_DIR)/$(BIN)";\
	fi

clean:
	rm -vf $(BIN) || sudo rm -vf $(BIN)

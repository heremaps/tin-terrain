
export CONTAINER_NAME ?= tin-terrain
export CLANG_FORMAT_BIN ?= clang-format

default:  build-docker

build-docker:
	docker build -t $(CONTAINER_NAME) .
clang-format:
	./scripts/clang-format.sh




    
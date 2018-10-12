
export CONTAINER_NAME ?= tin-terrain

default:  build-docker

build-docker:
	docker build -t $(CONTAINER_NAME) .
clang-format:
	./scripts/clang-format.sh




    
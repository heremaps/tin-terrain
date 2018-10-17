
export CONTAINER_NAME ?= tin-terrain
export TEST_CONTAINER_NAME ?= tin-terrain
export CLANG_FORMAT_BIN ?= clang-format

default:  build-docker

build-docker:
	docker build -t $(CONTAINER_NAME) .
docker-test:
	docker build -t $(TEST_CONTAINER_NAME) -f ./Dockerfile.tests .
	docker run --name $(TEST_CONTAINER_NAME) --rm -i -t $(TEST_CONTAINER_NAME)
clang-format:
	./scripts/clang-format.sh




    
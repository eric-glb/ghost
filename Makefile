SHELL := bash
MAKEFLAGS += --no-print-directory

PRG := ghost



.PHONY: help
help: # Print help on Makefile
	@echo -e "\nPlease use 'make <target>' where <target> is one of:\n"
	@grep '^[^.#]\+:\s\+.*#' Makefile | \
          sed "s/\(.\+\):\s*\(.*\) #\s*\(.*\)/`printf "\033[93m"`  \1`printf "\033[0m"`\t\3/" | \
          column -s $$'\t' -t
	@echo -e "\nCheck the Makefile to know exactly what each target is doing.\n"


.PHONY: build-upx
build-upx: # Build minimal Docker container image containing the compressed static binary
	docker build -f ./Dockerfile -t $(PRG) .
	@echo ""
	@docker images $(PRG)
	@echo -e "\nCommand to run: \e[1;32mdocker run --rm -t $(PRG)\e[0;m\n"

.PHONY: extract
extract: # extract compressed static binary from the built docker container image
	@docker images | grep -qE '$(PRG)\s+latest' || ( echo -e "\nERROR: image $(PRG):latest not found\n"; exit 1 )
	@docker create --name $(PRG)_extract $(PRG):latest >/dev/null
	@docker cp $(PRG)_extract:/$(PRG) ./$(PRG) &>/dev/null
	@docker rm $(PRG)_extract &>/dev/null
	@echo -e "\nExtracted binary: \e[1;32m./$(PRG)\e[0;m\n"

.PHONY: run
run: # Run the produced Docker container image
	@docker images | grep -qE '$(PRG)\s+latest' || make build-upx
	@docker images $(PRG) && sleep 2
	@docker run --rm -t $(PRG) /run -f 50

.PHONY: clean
clean: # # remove artefacts
	docker rmi $(PRG):latest &>/dev/null || true
	docker image prune -f &>/dev/null || true
	rm -f $(PRG)
	@echo ""

.PHONY: clean-all
clean-all: clean # remove artefacts and clean BuildKit cache
	docker buildx prune -f -a || true
	@echo ""

.PHONY: all 
all: clean build-upx extract # Clean, build, compress and extract binary


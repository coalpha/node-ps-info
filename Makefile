ifeq ($(OS), Windows_NT)
	gyp_cache := $(subst \,/,${HOME})/AppData/Local/node-gyp/Cache
else
	gyp_cache := ${HOME}/.cache/node-gyp
endif

node_include := $(word 1,$(wildcard $(gyp_cache)/*/))include/node

init: compile_flags.txt package-lock.json
	-

compile_flags.txt:
	$(file >$@)
	$(file >>$@,-Wall)
	$(file >>$@,-Wextra)
	$(file >>$@,-I$(node_include))

package-lock.json: package.json
	npm i

src := lib.c
bin := $(shell node -e "console.log(require('./package.json').main)")

$(bin): $(src)
	npx node-gyp rebuild

print: $(bin)
	node -e "console.log(require('./$<')())"

clean:
	npx node-gyp clean

.PHONY: compile_flags.txt print clean

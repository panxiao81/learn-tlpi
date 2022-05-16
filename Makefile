all: build example

build: download
	cd src && make
example: download
	cd tlpi-dist && make
download: download-code.py
	python3 download-code.py
venv: requirements.txt
	test -d venv || python3 -m venv venv
	. venv/bin/activate; pip install -r requirements.txt
docs: venv
	. venv/bin/activate; mkdocs build -f note/mkdocs.yml
docs-dev: venv
	. venv/bin/activate; mkdocs serve -f note/mkdocs.yml
clean-docs:
	-rm -rf note/site venv/
clean-example:
	-cd tlpi-dist && make clean

clean-build:
	cd src && make clean

clean: clean-docs clean-example clean-build
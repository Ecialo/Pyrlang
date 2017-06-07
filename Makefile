PY=python3

.PHONY: test run
run: build
	$(PY) test1.py

.PHONY: build
build:
	rm -f nativeETF.so; cd native_src && $(MAKE)

test: build
	$(PY) test/dist_etf_decode_test.py && \
	$(PY) test/dist_etf_transitive_test.py

test-debug: build
	gdb --args $(PY) test/dist_etf_decode_test.py

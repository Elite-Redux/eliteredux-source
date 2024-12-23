protoc.zip:
	$(shell wget -O protoc.zip https://github.com/protocolbuffers/protobuf/releases/download/v29.2/protoc-29.2-linux-x86_64.zip)

protoc: protoc.zip
	unzip -p protoc.zip bin/protoc > protoc

protobuf-java.jar:
	$(shell wget -o protobuf-java.jar https://repo1.maven.org/maven2/com/google/protobuf/protobuf-java/4.29.2/protobuf-java-4.29.2.jar)

codegen.jar: protoc protobuf-java.jar
	@echo "Compiling codegen"
	$(shell find ../../proto -name '*.proto' | xargs -I % protoc % --proto_path=./../../proto --java_out=src)
	$(shell find src/ -name '*.kt' -o -name '*.java' | xargs  kotlinc -cp "src/er/abilities:protobuf-java.jar" -include-runtime -d codegen.jar)

test:
	@echo $(PROTOS)

all: codegen.jar
	@echo "Codegen complete"

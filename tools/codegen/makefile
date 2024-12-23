all: generated codegen.jar
	@echo "Codegen complete"

protoc.zip:
	wget -O protoc.zip https://github.com/protocolbuffers/protobuf/releases/download/v29.2/protoc-29.2-linux-x86_64.zip && exit

protoc: protoc.zip
	unzip -p protoc.zip bin/protoc > protoc && exit

protobuf-java.jar:
	wget -O protobuf-java.jar https://repo1.maven.org/maven2/com/google/protobuf/protobuf-java/4.29.2/protobuf-java-4.29.2.jar && exit

KT_SRCS := $(wildcard src/*.kt src/*/*.kt src/*/*/*.kt)
PROTOS := ../../proto
PROTO_SRCS := $(wildcard $(PROTOS)/*.proto $(PROTOS)/*/*.proto $(PROTOS)/*/*/*.proto)

codegenkt.jar: protoc protobuf-java.jar $(KT_SRCS) $(PROTO_SRCS)
	find ../../proto -name '*.proto' | xargs -I % protoc % --proto_path=./../../proto --java_out=src && exit
	find src/ -name '*.kt' -o -name '*.java' | xargs  kotlinc -cp "protobuf-java.jar" -include-runtime -d codegenkt.jar && exit

codegen.jar: codegenkt.jar protobuf-java.jar
	rm -rf javabuild
	find src/ -name '*.java' | xargs  javac -cp "protobuf-java.jar:protobufkt.jar" -d javabuild && exit
	unzip -uoq protobuf-java.jar -d javabuild && exit
	unzip -uoq codegenkt.jar -d javabuild && exit
	jar cf codegen.jar -C javabuild . && exit

define GENERATE
../../$(strip $1): codegen.jar
	java -cp codegen.jar er.FileGenerator $2 $(realpath ../..)/$(strip $1)
ALL_GENERATED += ../../$(strip $1)
endef

$(eval $(call GENERATE, include/generated/constants/abilities.h,abilities))

generated: $(ALL_GENERATED)

removegenerated:
	find ../../generated -name "*.h" -type f -delete

regenerate: removegenerated generated

clean: removegenerated
	rm -rf javabuild
	rm -f protoc
	rm -f protoc.zip
	rm -f codegen.jar
	rm -f codegenkt.jar
	rm -f protobuf-java.jar

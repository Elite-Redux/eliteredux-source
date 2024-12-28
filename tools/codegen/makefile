KT_SRCS := $(wildcard src/er/*.kt src/er/*/*.kt src/er/*/*/*.kt)
PREPROC_SRCS := $(wildcard src/preproc/*.kt src/preproc/*/*.kt src/preproc/*/*/*.kt)
JAVA_SRCS := $(wildcard src/er/*.java src/er/*/*.java src/er/*/*/*.java)
PROTOS := ../../proto
PROTO_SRCS := $(subst ../../proto/,,$(wildcard $(PROTOS)/*.proto $(PROTOS)/*/*.proto $(PROTOS)/*/*/*.proto))
TEXTPROTO_SRCS := $(wildcard $(PROTOS)/*.textproto $(PROTOS)/*/*.textproto $(PROTOS)/*/*/*.textproto)

all: timestamp/generated
	@:

protoc:
	if [ ! -f protoc.zip ]; then wget -O protoc.zip https://github.com/protocolbuffers/protobuf/releases/download/v29.2/protoc-29.2-linux-x86_64.zip -q && exit; fi
	@unzip -p protoc.zip bin/protoc > protoc && exit

protobuf-java.jar:
	wget -O protobuf-java.jar https://repo1.maven.org/maven2/com/google/protobuf/protobuf-java/4.29.2/protobuf-java-4.29.2.jar -q && exit

protobuf-kotlin.jar:
	wget -O protobuf-kotlin.jar https://repo1.maven.org/maven2/com/google/protobuf/protobuf-kotlin/4.29.2/protobuf-kotlin-4.29.2.jar -q && exit

preproc.jar: $(PREPROC_SRCS) protobuf-java.jar
	find src/preproc -name '*.kt' | xargs  kotlinc -cp protobuf-java.jar  -include-runtime -d preproc.jar && exit 

define BUILD_PROTO
DEPS := $(shell grep -oPs '\S+\.proto(?=\\)' protodep/$1.protodep.txt)
timestamp/depsets/$1: ../../proto/$1 protoc
	@perl -pi.bak -e 'BEGIN{$$$$counter=1000000;} s/\@NEXT/$$$$counter++/ge' ../../proto/$1
	@rm -f ../../proto/$1.bak
	@mkdir -p timestamp/depsets
	./protoc ../../proto/$1 --proto_path=../../proto --java_out=src --descriptor_set_out=timestamp/depsets/$1 --experimental_allow_proto3_optional

protodep/$1.protodep.txt: timestamp/depsets/$1 protoc ../../proto/$1 $$(DEPS) preproc.jar protobuf-java.jar
	java -cp preproc.jar:protobuf-java.jar preproc.ProtoProcessor ../../proto/$1 timestamp/depsets/$1 && exit
	@mkdir -p protodep
	./protoc ../../proto/$1 --proto_path=../../proto --java_out=src --kotlin_out=src --dependency_out=protodep/$1.protodep.txt --experimental_allow_proto3_optional
endef
$(foreach proto,$(PROTO_SRCS),$(eval $(call BUILD_PROTO,$(proto))))

protodep: $(addsuffix .protodep.txt,$(addprefix protodep/,$(PROTO_SRCS)))

protos: protodep

codegen.jar: protobuf-java.jar $(JAVA_SRCS) protodep
	@rm -rf javabuild
	find src/er -name '*.java' | xargs  javac -cp "protobuf-java.jar:protobufkt.jar" -d javabuild && exit
	@jar cf codegen.jar -C javabuild . && exit

codegenkt.jar: protoc protobuf-java.jar codegen.jar protobuf-kotlin.jar $(KT_SRCS)
	find src/er -name '*.kt' | xargs  kotlinc -cp "protobuf-java.jar:protobuf-kotlin.jar:codegen.jar" -include-runtime -d codegenkt.jar && exit

define PREPROC_TEXTPROTO
$1: $2 preproc.jar protobuf-java.jar codegen.jar protoc
	@perl -pi.bak -e 'BEGIN{$$$$counter=1000000;} s/\@NEXT/$$$$counter++/ge' $2
	@./protoc ../../proto/$3.proto --encode=er.$3 --proto_path="../../proto/" --experimental_allow_proto3_optional < $2 > bin/$3.binpb;
	java -cp preproc.jar:protobuf-java.jar:codegen.jar preproc.TextprotoProcessor $2 bin/$3.binpb && exit
	@mkdir -p timestamp/preproc
	@touch $1
endef
TEXTPROTO_PREPROC := $(patsubst ../../proto/%.textproto,timestamp/preproc/%,$(TEXTPROTO_SRCS))
$(foreach textproto,$(TEXTPROTO_SRCS),$(eval $(call PREPROC_TEXTPROTO,$(patsubst ../../proto/%.textproto,timestamp/preproc/%,$(textproto)),$(textproto),$(patsubst ../../proto/%.textproto,%,$(textproto)))))


define GENERATE
../../$(strip $1): codegen.jar $(TEXTPROTO_SRCS) codegenkt.jar protobuf-java.jar protobuf-kotlin.jar $(TEXTPROTO_PREPROC)
	java -cp codegen.jar:codegenkt.jar:protobuf-java.jar:protobuf-kotlin.jar er.FileGenerator $2 $(realpath ../..)/$(strip $1)
ALL_GENERATED += ../../$(strip $1)
endef

$(eval $(call GENERATE, include/generated/constants/abilities.h, abilities))
$(eval $(call GENERATE, include/generated/constants/moves.h, moves))
$(eval $(call GENERATE, include/generated/constants/species.h, species))
$(eval $(call GENERATE, include/generated/data/pokemon/base_stats.h, basestats))
$(eval $(call GENERATE, include/generated/data/pokemon/evolution.h, evos))
$(eval $(call GENERATE, include/generated/data/text/species_names.h, speciesnames))
$(eval $(call GENERATE, include/generated/data/pokemon/tutor_learnsets.h, tutorlearnsets))
$(eval $(call GENERATE, include/generated/data/pokemon/level_up_learnset_pointers.h, leveluplearnsets))
$(eval $(call GENERATE, include/generated/data/pokemon/form_species_table_pointers.h, forms))
$(eval $(call GENERATE, include/generated/data/pokemon_graphics/icons.h, icons))
$(eval $(call GENERATE, include/generated/data/pokemon_graphics/pics.h, monpics))
$(eval $(call GENERATE, include/generated/data/pokemon_graphics/coordinates.h, coords))
$(eval $(call GENERATE, include/generated/data/pokemon_graphics/palettes.h, monpals))

timestamp/generated: $(ALL_GENERATED)
	@touch timestamp/generated

generated: timestamp/generated

removegenerated:
	rm -rf ../../include/generated

regenerate: removegenerated generated

cleanlocal: removegenerated
	rm -rf javabuild
	rm -rf protodep
	rm -f protoc
	rm -f protoc.zip
	rm -rf ../../generated
	rm -rf timestamp
	rm -f codegen.jar
	rm -f codegenkt.jar
	rm -rf out/depsets
	rm -rf out/descriptors

clean: cleanlocal
	rm -f *.jar

define ENCODE
echo "../../proto/$1.textproto";
./protoc ../../proto/$1.proto --encode=er.$1 --proto_path="../../proto/" --experimental_allow_proto3_optional < ../../proto/$1.textproto > bin/$1.binpb;
endef

binary: protoc
	mkdir -p bin/
	rm -rf bin/*.binpb
	$(foreach textproto,$(patsubst ../../proto/%.textproto,%,$(TEXTPROTO_SRCS)),$(call ENCODE,$(textproto)))

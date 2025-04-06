package preproc

import com.google.protobuf.DescriptorProtos.DescriptorProto
import com.google.protobuf.DescriptorProtos.EnumDescriptorProto
import com.google.protobuf.DescriptorProtos.FileDescriptorSet
import java.io.File

object ProtoProcessor {
    fun Collection<Int>.firstFree(n: Int, from: Int) =
        toSet().let { ids -> (from..size + from).filter { it !in ids }.take(n) }

    private fun EnumDescriptorProto.firstFree(n: Int) =
        valueList.map { it.number }.firstFree(n, 0)

    private fun DescriptorProto.firstFree(n: Int) =
        fieldList.map { it.number }.firstFree(n, 1)

    val BIG_INT = """(\d{7,})""".toRegex()

    @JvmStatic
    fun main(args: Array<String>) {
        val file = File(args.first())
        val descriptor = FileDescriptorSet.parseFrom(File(args[1]).inputStream())

        val toReplace = descriptor.fileList.flatMap { descriptorFile ->
            descriptorFile.messageTypeList.flatMap { checkMessage(it) } + descriptorFile.enumTypeList.flatMap {
                checkEnum(it)
            }
        }.toMap()

        if (toReplace.isEmpty()) return

        val lines = file.readText().replace(BIG_INT) { toReplace[it.value.toInt()]?.toString() ?: it.value }
        file.writer().use { it.write(lines) }
    }

    private fun checkMessage(descriptor: DescriptorProto): List<Pair<Int, Int>> =
        descriptor.fieldList.filter { it.number >= 1E7 }.map { it.number }
            .let { it.zip(descriptor.firstFree(it.size)) } + descriptor.nestedTypeList.flatMap { checkMessage(it) } + descriptor.enumTypeList.flatMap {
            checkEnum(
                it
            )
        }

    private fun checkEnum(descriptor: EnumDescriptorProto) =
        descriptor.valueList.filter { it.number >= 1E7 }.map { it.number }
            .let { it.zip(descriptor.firstFree(it.size)) }
}
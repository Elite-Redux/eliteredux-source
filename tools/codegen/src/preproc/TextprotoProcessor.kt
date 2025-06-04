package preproc

import com.google.protobuf.Descriptors.FieldDescriptor
import com.google.protobuf.Message
import preproc.ProtoProcessor.BIG_INT
import preproc.ProtoProcessor.firstFree
import java.io.File
import java.io.InputStream
import kotlin.system.measureTimeMillis

object TextprotoProcessor {
    private val PROTO_MESSAGE = """^\s*#\s*proto-message:\s+er\.(?<proto>[\w.]+)$""".toRegex()

    @JvmStatic
    fun main(args: Array<String>) {
        val file = File(args.first())
        val messageName =
            "er.proto." + file.useLines { lines -> lines.firstNotNullOf { PROTO_MESSAGE.find(it) } }.groups["proto"]!!.value

        @Suppress("UNCHECKED_CAST")
        val clazz = Class.forName(messageName) as Class<Message>
        val message: Message = clazz.getMethod("parseFrom", InputStream::class.java)
            .invoke(null, File(args[1]).inputStream()) as Message
        val allValues: Map<FieldDescriptor, List<Int>> = message.allFields.asSequence().flatMap { field ->
            visitField(field.key, field.value) { fieldDescriptor, value ->
                when (fieldDescriptor.javaType) {
                    FieldDescriptor.JavaType.INT -> value as? Int
                    FieldDescriptor.JavaType.LONG -> (value as? Long)?.toInt()
                    else -> null
                }?.let { fieldDescriptor to it }
            }
        }.groupBy({ it.first }, { it.second })

        val toUpdate =
            allValues.mapValues { (_, values) -> values.filter { it >= 1E7 } }.filterValues { it.isNotEmpty() }

        if (toUpdate.isEmpty()) return
        val replacements: Map<Int, Int> = toUpdate.flatMap { (descriptor, values) ->
            values.zip(
                allValues[descriptor]!!.firstFree(
                    values.size,
                    allValues[descriptor]?.min()?.takeIf { it < 1E7 } ?: 1))
        }.toMap()

        val lines = file.readText().replace(BIG_INT) { replacements[it.value.toInt()]?.toString() ?: it.value }
        file.writer().use { it.write(lines) }
    }

    private fun <T> visitField(
        descriptor: FieldDescriptor,
        value: Any?,
        block: (FieldDescriptor, Any?) -> T?
    ): Sequence<T> =
        when {
            value is Iterable<*> -> value.asSequence().flatMap { visitField(descriptor, it, block) }

            descriptor.type == FieldDescriptor.Type.MESSAGE -> (value as Message).allFields.asSequence().flatMap {
                visitField(it.key, it.value, block)
            }

            else -> listOfNotNull(block(descriptor, value)).asSequence()
        }
}
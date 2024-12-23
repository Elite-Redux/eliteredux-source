package er

import er.abilities.AbilitiesEnumGenerator
import java.io.File

object FileGenerator {
    @JvmStatic
    fun main(args: Array<String>) {
        val (type, file) = args
        try {
            File(file).parentFile.mkdirs()
            when (type) {
                "abilities" -> AbilitiesEnumGenerator.generate(file)
                "moves" -> AbilitiesEnumGenerator.generate(file)
                else -> error("Invalid file type $type")
            }
        } catch (e: Exception) {
            throw Exception("Failed processing $type generating file $file", e)
        }
    }
}
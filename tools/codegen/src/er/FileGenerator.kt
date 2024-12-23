package er

import er.abilities.AbilitiesEnumGenerator

object FileGenerator {
    @JvmStatic
    fun main(args: Array<String>) {
        val (type, file) = args
        when (type) {
            "abilities" -> AbilitiesEnumGenerator.generate(file)
            else -> error("Invalid file type ${type}")
        }
    }
}
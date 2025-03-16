package er.text

import com.google.protobuf.TextFormat
import er.FileGenerator.IND
import er.Generator
import er.proto.HelpArticles
import er.text.FontMapping.Font.SMALL_NARROW
import er.text.FontMapping.breakString
import java.io.File
import java.io.OutputStreamWriter

object ArticlesGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val categories = TextFormat.parse(
            File("../../proto/HelpArticles.textproto").readText(),
            HelpArticles::class.java
        ).helpCategoryList

        writer.appendLine(
            """
            |#define NUM_HELP_ARTICLES ${categories.size}
            |
            |const HelpCategory gHelpArticles[NUM_HELP_ARTICLES] = {""".trimMargin()
        )

        for (category in categories) {
            writer.appendLine(
                """
                |$IND{
                |$IND$IND.title = $("${category.title}"),
                |$IND$IND.color = MENU_COLOR_${category.color},
                |$IND$IND.numEntries = ${category.helpArticleCount},
                |$IND$IND.entries = (const HelpArticle[]) {""".trimMargin()
            )
            for (article in category.helpArticleList) {
                writer.appendLine(
                    """
                    |$IND$IND$IND{
                    |$IND$IND$IND$IND.title = $("${article.title}"),
                    |$IND$IND$IND$IND.numPages = ${article.pageCount},
                    |$IND$IND$IND$IND.pages = (const u8* const[]) {
                    |$IND$IND$IND$IND$IND${
                            article.pageList.joinToString("\n$IND$IND$IND$IND$IND") {
                                """$("${
                                    breakString(
                                        it,
                                        SMALL_NARROW,
                                        210
                                    )
                                }"),"""
                            }
                        }
                    |$IND$IND$IND$IND}
                    |$IND$IND$IND},""".trimMargin()
                )
            }
            writer.appendLine(
                """
                |$IND$IND}
                |$IND},""".trimMargin()
            )
        }

        writer.appendLine("};")
    }
}
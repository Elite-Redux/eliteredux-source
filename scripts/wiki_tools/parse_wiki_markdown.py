#!/usr/bin/env python3
"""
Elite Redux Wiki Parser
Converts markdown documentation to protobuf text format for the in-game wiki
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple


class WikiEntry:
    def __init__(self, title: str):
        self.title = title
        self.raw_lines = []
        self.pages = []
    
    def add_line(self, line: str):
        """Add a content line to this entry"""
        self.raw_lines.append(line)
    
    def paginate(self, lines_per_page: int = 9, max_pages: int = 5):
        """Split content into pages respecting constraints"""
        self.pages = []
        current_page = []
        
        for line in self.raw_lines:
            current_page.append(line)
            if len(current_page) >= lines_per_page:
                self.pages.append(current_page)
                current_page = []
                if len(self.pages) >= max_pages:
                    print(f"WARNING: Entry '{self.title}' exceeds {max_pages} pages. Content truncated.")
                    return
        
        # Add remaining lines as the last page
        if current_page:
            self.pages.append(current_page)


class WikiCategory:
    def __init__(self, title: str, color: str = "RED"):
        self.title = title
        self.color = color
        self.entries = []
    
    def add_entry(self, entry: WikiEntry):
        self.entries.append(entry)


class WikiParser:
    def __init__(self, markdown_path: str):
        self.markdown_path = Path(markdown_path)
        self.categories = []
        self.current_category = None
        self.current_entry = None
        self.in_wiki_content = False
    
    def parse(self) -> List[WikiCategory]:
        """Parse the markdown file and return structured data"""
        with open(self.markdown_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
        
        for line in lines:
            line = line.rstrip('\n')
            
            # Look for the start of wiki content
            if line.strip() == "# Actual Wiki Content":
                self.in_wiki_content = True
                continue
            
            if not self.in_wiki_content:
                continue
            
            # Category detection (## lines)
            if line.startswith("## "):
                self._handle_category(line)
            
            # Entry detection (### lines)
            elif line.startswith("### "):
                self._handle_entry(line)
            
            # Content line detection (* lines)
            elif line.startswith("* "):
                self._handle_content_line(line)
            
            # Blank line (just *)
            elif line.strip() == "*":
                self._handle_blank_line()
        
        # Finalize last entry and category
        self._finalize_current_entry()
        self._finalize_current_category()
        
        return self.categories
    
    def _handle_category(self, line: str):
        """Process a category header line"""
        # Finalize previous category and entry
        self._finalize_current_entry()
        self._finalize_current_category()
        
        # Extract category title
        title = line[3:].strip()
        
        # Assign colors based on category (customizable)
        color_map = {
            "Basic Information": "BLUE",
            "Ability Information": "GREEN",
            "Battle Information": "RED",
            "Miscellaneous Information": "YELLOW"
        }
        color = color_map.get(title, "RED")
        
        # Clean the title
        title = self._clean_markdown(title)
        
        self.current_category = WikiCategory(title, color)
        print(f"Found category: {title} (color: {color})")
    
    def _handle_entry(self, line: str):
        """Process an entry header line"""
        # Finalize previous entry
        self._finalize_current_entry()
        
        # Extract entry title
        title = line[4:].strip()
        
        # Clean the title
        title = self._clean_markdown(title)
        
        if self.current_category is None:
            print(f"WARNING: Entry '{title}' found without a category. Skipping.")
            return
        
        self.current_entry = WikiEntry(title)
        print(f"  Found entry: {title}")
    
    def _handle_content_line(self, line: str):
        """Process a content line"""
        if self.current_entry is None:
            return
        
        # Extract content after "* "
        content = line[2:]
        
        # Clean up markdown formatting
        content = self._clean_markdown(content)
        
        self.current_entry.add_line(content)
    
    def _handle_blank_line(self):
        """Process a blank line (just *)"""
        if self.current_entry is None:
            return
        
        # Add empty string for blank line
        self.current_entry.add_line("")
    
    def _clean_markdown(self, text: str) -> str:
        """Remove markdown formatting and problematic characters"""
        # Remove bold markers
        text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
        
        # Remove links, keep just the text
        text = re.sub(r'\[([^\]]+)\]\([^\)]+\)', r'\1', text)
        
        # Remove other markdown artifacts
        text = text.replace('&', 'and')
        
        # Replace problematic characters for GBA font system
        replacements = {
            '~': 'about',
            'é': 'e',
            'É': 'E', 
            'à': 'a',
            'è': 'e',
            'ù': 'u',
            'ñ': 'n',
            'ó': 'o',
            'í': 'i',
            'á': 'a',
            'ú': 'u',
            ''': "'",
            ''': "'",
            '"': '"',
            '"': '"',
            '—': '-',
            '–': '-',
            '…': '...',
            '•': '*',
            '×': 'x',
            '÷': '/',
            '≤': '<=',
            '≥': '>=',
            '≠': '!=',
            '±': '+/-',
            '°': ' degrees',
            '™': 'TM',
            '®': '(R)',
            '©': '(C)',
            '€': 'EUR',
            '£': 'GBP',
            '¥': 'JPY',
            '¢': 'cents',
            '§': 'S',
            '¶': 'P',
            '†': '+',
            '‡': '++',
            '¿': '?',
            '¡': '!',
            'Pokémon': 'Pokemon',
            'Pokédex': 'Pokedex',
            'Poké': 'Poke',
        }
        
        for old, new in replacements.items():
            text = text.replace(old, new)
        
        # Remove any remaining non-ASCII characters
        # Replace with apostrophe for common cases, otherwise remove
        cleaned_text = []
        for char in text:
            if ord(char) < 128:
                cleaned_text.append(char)
            elif char in [''', ''', '`', '´']:
                cleaned_text.append("'")
            # Skip other non-ASCII characters
        text = ''.join(cleaned_text)
        
        return text
    
    def _finalize_current_entry(self):
        """Finalize and add current entry to category"""
        if self.current_entry and self.current_category:
            self.current_entry.paginate()
            self.current_category.add_entry(self.current_entry)
            print(f"    Entry has {len(self.current_entry.pages)} pages")
            self.current_entry = None
    
    def _finalize_current_category(self):
        """Finalize and add current category to list"""
        if self.current_category:
            self.categories.append(self.current_category)
            print(f"  Category has {len(self.current_category.entries)} entries")
            self.current_category = None


class ProtoFormatter:
    """Formats wiki data into protobuf text format"""
    
    @staticmethod
    def format_categories(categories: List[WikiCategory]) -> str:
        """Convert categories to protobuf text format"""
        output = ["# proto-file: HelpArticles.proto",
                  "# proto-message: er.HelpArticles",
                  ""]
        
        for category in categories:
            output.append("help_category {")
            output.append(f'  title: "{category.title}"')
            output.append(f'  color: {category.color}')
            
            for entry in category.entries:
                output.append('  help_article {')
                output.append(f'    title: "{entry.title}"')
                
                for page_num, page_lines in enumerate(entry.pages):
                    # Combine lines for this page
                    page_content = ProtoFormatter._format_page(page_lines)
                    output.append(f'    page: {page_content}')
                
                output.append('  }')
            
            output.append('}')
        
        return '\n'.join(output)
    
    @staticmethod
    def _format_page(lines: List[str]) -> str:
        """Format a page's lines for protobuf"""
        # Join lines with \n, handling empty lines
        formatted_lines = []
        for i, line in enumerate(lines):
            if i == 0:
                # First line doesn't need \n prefix
                formatted_lines.append(line)
            else:
                # Subsequent lines need \n prefix
                if line:  # Non-empty line
                    formatted_lines.append(f"\\n{line}")
                else:  # Empty line
                    formatted_lines.append("\\n")
        
        # Combine all lines and wrap in quotes
        page_text = ''.join(formatted_lines)
        
        # Escape any quotes in the text
        page_text = page_text.replace('"', '\\"')
        
        return f'"{page_text}"'


def main():
    # Paths
    project_root = Path(__file__).parent.parent.parent
    markdown_path = project_root / "docs" / "er-wiki-google-docs.md"
    output_path = project_root / "proto" / "HelpArticles.textproto"
    
    print(f"Elite Redux Wiki Parser")
    print(f"======================")
    print(f"Input: {markdown_path}")
    print(f"Output: {output_path}")
    print()
    
    # Check if input exists
    if not markdown_path.exists():
        print(f"ERROR: Input file not found: {markdown_path}")
        sys.exit(1)
    
    # Parse markdown
    parser = WikiParser(markdown_path)
    categories = parser.parse()
    
    print()
    print(f"Parsing complete!")
    print(f"Found {len(categories)} categories")
    print(f"Total entries: {sum(len(cat.entries) for cat in categories)}")
    print()
    
    # Format to protobuf
    formatter = ProtoFormatter()
    protobuf_content = formatter.format_categories(categories)
    
    # Save output
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(protobuf_content)
    
    print(f"Wiki content written to: {output_path}")
    print("Done! Remember to rebuild the ROM to see changes in-game.")


if __name__ == "__main__":
    main()
import itertools
import re


def process_markdown_to_bikeshed(md_content, filename=None):
    """
    Process Markdown content to make it Bikeshed compliant.
    """
    def replace_headers(match):
        level = len(match.group(1)) - 1  # AV1 markdown files start with level 2
        header_text = match.group(2)
        header_id = header_text.replace(" ", "_").lower()
        level = min(level, 6)
        if level == 0:
            return f'<h1>{header_text}</h1>'
        return f'{"#"*level} {header_text} {"#"*level} {{#{header_id}}}'

    def convert_table(markdown_table):
        """
        Converts GitHub flavored markdown text containing zero or more tables to Bikeshed formatted text.
        """
        lines = markdown_table.strip().split('\n')
        headers = []
        alignments = []
        rows = []

        if lines:
            headers = [header.strip() for header in lines[0].split('|') if header.strip()]
        if len(lines) > 1:
            raw_alignments = [align.strip() for align in lines[1].split('|') if align.strip()]
            for align in raw_alignments:
                if align.startswith(':') and align.endswith(':'):
                    alignments.append('{:^}')
                elif align.endswith(':'):
                    alignments.append('{:>}')
                else:
                    alignments.append('{:<}')
        for line in lines[1:]:
            rows.append([cell for cell in line.split('|') if cell.strip()])

        # TODO: get table header
        bikeshed_table = '```cpp\n'
        for row in rows:
            if len(row) == 0:
                continue
            row[0] = row[0].replace('@@', '').replace('{:.syntax }', '')
            row[0] = row[0].replace('\\>\\>', '>>')
            row[0] = row[0].replace('\\<\\<', '<<')
            row[0] = row[0].replace('\\-\\-', '-- ')
            if len(row) == 2:
                if 'Type' in row[1]:
                    bikeshed_table += row[0].strip() + '\n'
                    continue
                spaces_cnt = sum(1 for _ in itertools.takewhile(str.isspace, row[0]))
                spaces = ' '*spaces_cnt
                bikeshed_table += f'{spaces}{row[1]} {row[0].strip()}\n'
            else:
                bikeshed_table += row[0] + '\n'
        bikeshed_table += '```\n'
        return bikeshed_table


    # Convert Markdown headers to Bikeshed-compatible format with manually-specified ID
    processed_content = re.sub(r'^(#{1,5})\s+(.+)$', replace_headers, md_content, flags=re.MULTILINE)

    # Split markdown text by identifying potential table start and end markers
    parts = processed_content.split('\n\n')
    bikeshed_text = ""
    for part in parts:
        # Check if the part is a table by looking for at least two lines with pipe characters
        if part.count('|') > 1 and '\n' in part:
            bikeshed_text += convert_table(part) + '\n\n'
        else:
            bikeshed_text += part + '\n\n'

    return bikeshed_text


# Во имя Отца и Сына и Святого Духа...
if __name__ == '__main__':
    md_file = '../06.bitstream.syntax.md'

    with open(md_file, 'r', encoding='utf-8') as file:
        md_content = file.read()
        processed_content = process_markdown_to_bikeshed(md_content, md_file)

        print(processed_content)

# line numbers
# https://codepen.io/heiswayi/pen/jyKYyg

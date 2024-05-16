import subprocess
import os
import re


def get_definitions(md_content):
    """
    Get definitions in an array [{name: ..., description: ...}, ...]
    """
    # Define the regex pattern to match term names followed by their definitions.
    # This pattern accounts for a term name followed by a newline and ": ", then captures the subsequent text
    # as the definition until the next term or the end of the text.
    pattern = re.compile(r'^(.*?)\n:\s(.*?)(?=\n\n|$)', re.MULTILINE | re.DOTALL)

    matches = pattern.findall(md_content)
    definitions = []

    for match in matches:
        term_name = match[0].strip().split('\n')[-1]  # Get the last line before ": " as term name
        definition_text = match[1].strip()
        # Fix notes
        # definition_text = re.sub(r'\*\*Note:\*\*.*', 'NOTE: ', definition_text, flags=re.DOTALL).strip()

        definitions.append({
            "name": term_name,
            "description": definition_text
        })

    return definitions


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
        for line in lines[2:]:
            rows.append([cell.strip() for cell in line.split('|') if cell.strip()])

        # TODO: get table header
        bikeshed_table = 'Table: TBD\n'
        if headers:
            bikeshed_table += '    ' + ' | '.join(headers) + '\n'
        bikeshed_table += '    ' + '--|' * len(headers) + '\n'
        for row in rows:
            bikeshed_table += '    ' + ' | '.join(row) + '\n'

        return bikeshed_table

    # Deal with definitions
    if '02.terms' in filename:
        md_content = re.sub(r'\n\s\s', ' ', md_content) # fix indentation
        terms = get_definitions(md_content)
        processed_content = '# Terms and definitions # {#terms_and_definitions}\n\nFor the purposes of this document, the following terms and definitions apply:\n\n'
        for term in terms:
            processed_content += f'<dfn>{term["name"]}</dfn>\n\n{term["description"]}\n\n'
        # deal with notes
        processed_content = processed_content.replace('**Note:**', '\n\nNOTE:')
        processed_content = processed_content.replace('{:.alert .alert-info }', '')
    else:
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


def generate_spec(header_path, markdown_files, output_bs_path='index.bs'):
    """
    Generate the Bikeshed specification file and execute bikeshed tool.
    """
    try:
        # Read the header content
        with open(header_path, 'r', encoding='utf-8') as file:
            content = file.read()

        # Process and concatenate the content of each Markdown file
        for md_file in markdown_files:
            with open(md_file, 'r', encoding='utf-8') as file:
                md_content = file.read()
                processed_content = process_markdown_to_bikeshed(md_content, md_file)
                content += '\n' + processed_content

        # Write the combined content to index.bs
        with open(output_bs_path, 'w', encoding='utf-8') as file:
            file.write(content)

        print(f'Generated {output_bs_path} successfully.')

        # Run the bikeshed command to generate the HTML file
        subprocess.run(['bikeshed', 'spec', output_bs_path], check=True)
        print('Bikeshed compilation completed successfully.')

    except FileNotFoundError as e:
        print(f'Error: {e.strerror} - {e.filename}')
    except subprocess.CalledProcessError as e:
        print(f'Error: Bikeshed compilation failed with status {e.returncode}')
    except Exception as e:
        print(f'An unexpected error occurred: {str(e)}')


# Во имя Отца и Сына и Святого Духа...
if __name__ == '__main__':
    header_path = './index_header.bs'
    markdown_files = ['../01.scope.md',
                      '../02.terms.md',
                      '../03.symbols.md',
                      '../04.conventions.md',]
                      # '../06.bitstream.syntax.md',
                      # '../07.bitstream.semantics.md']
    generate_spec(header_path, markdown_files)

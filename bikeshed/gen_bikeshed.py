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

    return processed_content


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
                      '../04.conventions.md',
                      '../06.bitstream.syntax.md',
                      '../07.bitstream.semantics.md']
    generate_spec(header_path, markdown_files)

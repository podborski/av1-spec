import os
import re

TERMS_FILE = '02_Terms_and_definitions.bs'

BS_FILES = [
    '03_Symbols_and_abbreviated_terms.bs',
    '04_Conventions.bs',
    '05_Syntax_structures.bs',
    '06_Syntax_structures_semantics.bs',
    '07_Decoding_process.bs',
    '08_Parsing_process.bs',
    '09_Additional_tables.bs',
    '10_Annex_A_Profiles_and_levels.bs',
    '11_Annex_B_Length_delimited_bitstream_format.bs',
    '12_Annex_C_Error_resilience_behavior_inf.bs',
    '13_Annex_D_Large_scale_tile_use_case_inf.bs',
    '14_Annex_E_Decoder_model.bs',
    '15_Bibliography.bs']


def get_terms():
    with open(os.path.join('bikeshed', TERMS_FILE), 'r', encoding='utf-8') as infile:
        section2_text = infile.read()

    # Regular expression to find all <dfn>...</dfn> tags and capture the content
    pattern = re.compile(r'<dfn>(.*?)</dfn>', re.DOTALL)
    definitions = pattern.findall(section2_text)

    # Sort definitions by length in descending order
    definitions.sort(key=len, reverse=True)

    return definitions


def find_and_use_definitions():
    definitions = get_terms()
    for bs_file in BS_FILES:
        bs_path = os.path.join('bikeshed', bs_file)
        with open(bs_path, 'r', encoding='utf-8') as infile:
            content = infile.read()

        # Split content into lines
        lines = content.split('\n')
        updated_lines = []

        def make_replace_with_link(term):
            # Create a replacement function for a specific term (case-insensitive)
            pattern = re.compile(r'\b' + re.escape(term) + r'\b', re.IGNORECASE)
            def replace_with_link(match):
                # Use the original case from the definition
                return f'__PLACEHOLDER_{term}__'
            return lambda line: pattern.sub(replace_with_link, line)

        # Apply each definition in the correct order
        for line in lines:
            if re.match(r'^\s*#{1,6}\s', line):
                # This line is a section heading, so don't replace terms
                updated_lines.append(line)
            else:
                # Replace terms in this line, processing definitions in order
                for term in definitions:
                    replace_with_link = make_replace_with_link(term)
                    line = replace_with_link(line)
                updated_lines.append(line)

        updated_content = '\n'.join(updated_lines)

        # Replace placeholders with final links
        for term in definitions:
            placeholder_pattern = re.compile(r'__PLACEHOLDER_' + re.escape(term) + r'__')
            updated_content = placeholder_pattern.sub(f'[={term}=]', updated_content)

        with open(bs_path, 'w', encoding='utf-8') as outfile:
            outfile.write(updated_content)


if __name__ == '__main__':
    find_and_use_definitions()

#!/usr/bin/env python3
"""Convert remaining K&R-style C function definitions to ANSI C.

Approach: for each K&R function definition found, convert only the
parameter list and declaration lines. Return types on preceding lines
are left untouched (they remain on their own line, which is valid C).
"""
import re, sys

C_KEYWORDS = {
    'if', 'while', 'for', 'switch', 'sizeof', 'return', 'do', 'else',
    'case', 'default', 'break', 'continue', 'goto', 'typedef', 'struct',
    'union', 'enum', 'extern', 'static', 'auto', 'register', 'const',
    'volatile', 'unsigned', 'signed', 'int', 'char', 'short', 'long',
    'float', 'double', 'void'
}

TYPE_KW = re.compile(
    r'\b(?:register\s+)?(?:int|char|Char|unsigned|long|short|double|float|'
    r'signed|const|struct|void|BUFFER|WINDOW|LINE|MC|ENVAR|Ckey|FILE|'
    r'KSTREAM|KFILE|KS_FLAG|KEY|Cstr|size_t|Bool)\b'
)


def strip_comment(line):
    """Strip trailing C comment from a line (single-line /* ... */ only)."""
    result = []
    in_string = False
    in_char = False
    i = 0
    while i < len(line):
        if line[i] == '"' and not in_char:
            in_string = not in_string
        elif line[i] == "'" and not in_string:
            in_char = not in_char
        if not in_string and not in_char:
            if i <= len(line) - 2 and line[i:i+2] == '/*':
                end = line.find('*/', i + 2)
                if end != -1:
                    i = end + 2
                    continue
        result.append(line[i])
        i += 1
    return ''.join(result)


def parse_decl_part(part, base_type=None):
    """Parse a single declaration fragment like 'Char *str' or '*src'."""
    part = part.strip()
    if not part:
        return None
    m = re.match(r'^(.*?)(\**)\s*(\w+)(\s*\[\s*\])?\s*$', part)
    if not m:
        return None
    type_prefix = m.group(1).strip()
    stars = m.group(2) or ''
    name = m.group(3)
    arr = m.group(4) or ''
    if type_prefix:
        sep = '' if type_prefix.endswith('*') else ' '
        full = type_prefix + sep + stars + name + arr
    elif base_type:
        full = base_type + ' ' + stars + name + arr
    else:
        full = stars + name + arr
    return (full, name)


def parse_param_decls(decl_lines):
    """Parse K&R parameter declaration lines.

    Returns list of (full_type_with_stars, param_name).
    Handles comma-separated declarations and register qualifier.
    """
    results = []
    base_type = None
    for dl in decl_lines:
        stripped = strip_comment(dl).rstrip()
        if not stripped.endswith(';'):
            continue
        stripped = stripped.rstrip(';').strip()
        if not stripped:
            continue
        # Strip leading 'register' from the declaration
        register_prefix = ''
        m_reg = re.match(r'^(register\s+)(.*)', stripped)
        if m_reg:
            register_prefix = 'register '
            stripped = m_reg.group(2).strip()
        parts = [p.strip() for p in stripped.split(',')]
        first = parse_decl_part(parts[0], base_type)
        if not first:
            continue
        first_full, first_name = first
        # Prepend register if present
        if register_prefix:
            first_full = register_prefix + first_full
        results.append((first_full, first_name))
        # Extract base type for subsequent comma-separated params
        idx = first_full.find(first_name)
        if idx >= 0:
            before = first_full[:idx].rstrip()
            base_type = re.sub(r'\*+\s*$', '', before).rstrip()
            if register_prefix:
                base_type = re.sub(r'^register\s+', '', base_type)
        for part in parts[1:]:
            result = parse_decl_part(part, base_type)
            if result:
                r_full, r_name = result
                if register_prefix:
                    r_full = register_prefix + r_full
                results.append((r_full, r_name))
    return results


def is_kr_function(lines, i):
    """Check if line i is a K&R-style function definition.
    Returns dict with info, or None."""
    line = lines[i].rstrip('\n')
    code = strip_comment(line).rstrip()

    m = re.match(r'^(\s*)(\w+)\s*\(([^)]*)\)\s*$', code)
    if not m:
        return None

    funcname = m.group(2)
    if funcname in C_KEYWORDS:
        return None

    params_str = m.group(3).strip()
    if not params_str:
        return None

    # Skip if already ANSI (params contain types or *)
    if '*' in params_str or TYPE_KW.search(params_str):
        return None

    param_names = [p.strip() for p in params_str.split(',') if p.strip()]
    indent_str = m.group(1)

    # Collect param declarations on following non-blank lines
    j = i + 1
    decl_lines = []
    while j < len(lines):
        nl = lines[j].rstrip('\n')
        stripped = nl.strip()
        if not stripped:
            break
        if stripped.startswith('#'):
            break
        if stripped == '{':
            break
        decl_check = strip_comment(nl).rstrip()
        if decl_check.endswith(';') and TYPE_KW.search(decl_check):
            decl_lines.append(nl)
            j += 1
        else:
            break

    # Skip blank lines after declarations
    decl_end = j
    while decl_end < len(lines) and not lines[decl_end].strip():
        decl_end += 1

    # Check if opening brace follows
    if decl_end < len(lines) and lines[decl_end].strip() == '{':
        if decl_lines:
            return {
                'funcname': funcname,
                'params_str': params_str,
                'param_names': param_names,
                'indent': indent_str,
                'header_idx': i,
                'decl_start': i + 1,
                'decl_end': j,       # last decl line index + 1 = first non-decl after decls
                'brace_idx': decl_end,
                'decl_lines': decl_lines,
                'has_decls': True,
            }
        elif param_names:
            # Implicit int - no declarations, brace follows
            return {
                'funcname': funcname,
                'params_str': params_str,
                'param_names': param_names,
                'indent': indent_str,
                'header_idx': i,
                'decl_start': i + 1,
                'decl_end': j,
                'brace_idx': decl_end,
                'decl_lines': [],
                'has_decls': False,
            }
    return None


def generate_ansi_params(param_names, decl_lines):
    """Generate ANSI C parameter string from K&R params and declarations."""
    if decl_lines:
        parsed = parse_param_decls(decl_lines)
        type_map = {}
        for full_decl, param_name in parsed:
            type_map[param_name] = full_decl
        ansi_params = []
        for pn in param_names:
            if pn == '...':
                ansi_params.append('...')
            elif pn in type_map:
                ansi_params.append(type_map[pn])
            else:
                ansi_params.append('int ' + pn)
        return ', '.join(ansi_params)
    else:
        # Implicit int
        return ', '.join('int ' + pn for pn in param_names if pn)


def convert_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()

    modifications = []
    i = 0
    while i < len(lines):
        info = is_kr_function(lines, i)
        if info:
            header_line_idx = info['header_idx']
            ansi_params = generate_ansi_params(info['param_names'], info['decl_lines'])
            
            # Build the new header line (preserve indentation and any trailing comment)
            # Extract trailing comment from original line if present
            original_line = lines[header_line_idx].rstrip('\n')
            code_part = strip_comment(original_line).rstrip()
            comment_match = re.search(r'(/\*.*?\*/)\s*$', original_line.strip())
            trailing_comment = '  ' + comment_match.group(1) if comment_match else ''
            
            # New header: indent + funcname(ansi_params) + optional comment
            new_header = info['indent'] + info['funcname'] + '(' + ansi_params + ')' + trailing_comment + '\n'

            # Replace: header line + decl lines -> new header + brace
            # Lines to replace: from header_line_idx to brace_idx (inclusive)
            start = header_line_idx
            end = info['brace_idx']
            brace_line = lines[info['brace_idx']]
            new_lines = [new_header, brace_line]

            modifications.append((start, end, new_lines))
            i = info['brace_idx'] + 1
        else:
            i += 1

    if modifications:
        for start, end, new_lines in reversed(modifications):
            lines[start:end + 1] = new_lines

        with open(filepath, 'w') as f:
            f.writelines(lines)

        print(f'  {filepath}: {len(modifications)} functions converted')
    else:
        print(f'  {filepath}: no K&R functions found')


if __name__ == '__main__':
    for filepath in sys.argv[1:]:
        try:
            convert_file(filepath)
        except Exception as e:
            print(f'  ERROR in {filepath}: {e}', file=sys.stderr)

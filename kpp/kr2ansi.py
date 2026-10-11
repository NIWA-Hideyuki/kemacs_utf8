#!/usr/bin/env python3
"""Convert K&R-style C function definitions to ANSI C in .C_ files."""
import re, sys

C_KEYWORDS = set(['if', 'while', 'for', 'switch', 'sizeof', 'return',
    'do', 'else', 'case', 'default', 'break', 'continue', 'goto',
    'typedef', 'struct', 'union', 'enum', 'extern', 'static',
    'auto', 'register', 'const', 'volatile', 'unsigned', 'signed',
    'int', 'char', 'short', 'long', 'float', 'double', 'void'])

TYPE_PATTERN = r'(?:int|char|Char|unsigned|long|short|double|float|signed|const|struct|void|BUFFER|WINDOW|LINE|MC|ENVAR|Ckey|FILE|KSTREAM|KFILE|KS_FLAG|KEY|Cstr)'
TYPE_KW_RE = re.compile(r'\b' + TYPE_PATTERN + r'\b')

def is_already_ansi(params_str):
    if not params_str:
        return False
    if re.search(r'\bvoid\b', params_str) or '*' in params_str:
        return True
    if TYPE_KW_RE.search(params_str):
        return True
    if re.search(r'[!&|<>{}()=+\-*/%]', params_str):
        return True
    return False

def parse_decl_part(part, base_type=None):
    part = re.sub(r'/\*.*?\*/', '', part).strip()
    if not part:
        return None
    m = re.match(r'^(.*?)\s*(\**)\s*(\w+)(\s*\[\s*\])?\s*$', part)
    if not m:
        return None
    type_prefix = m.group(1).strip()
    stars = m.group(2) or ''
    name = m.group(3)
    array = m.group(4) or ''
    if type_prefix:
        full = type_prefix + ' ' + stars + name + array
    elif base_type:
        full = base_type + ' ' + stars + name + array
    else:
        full = name + array
    return (full, name)

def parse_param_decl(line):
    stripped = line.strip()
    stripped = re.sub(r'/\*.*?\*/', '', stripped).rstrip()
    if not stripped.endswith(';'):
        return None
    stripped = stripped.rstrip(';').strip()
    if not stripped:
        return None
    parts = [p.strip() for p in stripped.split(',')]
    results = []
    first = parse_decl_part(parts[0])
    if not first:
        return None
    results.append(first)
    first_full, first_name = first
    idx = first_full.rfind(first_name)
    base_type = first_full[:idx].rstrip() if idx >= 0 else ''
    base_type = re.sub(r'\*+\s*$', '', base_type).rstrip()
    for part in parts[1:]:
        result = parse_decl_part(part, base_type)
        if result:
            results.append(result)
    return results

def convert_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    modified = []
    i = 0
    changes = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r'^(\s*)(\w+)\s*\(([^)]*)\)\s*$', line.rstrip('\n'))
        if m and m.group(2) not in C_KEYWORDS and not is_already_ansi(m.group(3)):
            indent = m.group(1)
            funcname = m.group(2)
            params_str = m.group(3).strip()
            param_names = []
            if params_str:
                for p in params_str.split(','):
                    p = p.strip()
                    if not p:
                        continue
                    nm = re.search(r'(\w+)\s*$', p)
                    if nm:
                        param_names.append(nm.group(1))
            j = i + 1
            param_decls = []
            while j < len(lines):
                next_line = lines[j].strip()
                if not next_line:
                    break
                if next_line == '{':
                    break
                if next_line.startswith('#'):
                    break
                decl = parse_param_decl(lines[j])
                if decl:
                    param_decls.extend(decl)
                    j += 1
                else:
                    break
            while j < len(lines) and not lines[j].strip():
                j += 1
            if j < len(lines) and lines[j].strip() == '{' and (param_decls or param_names):
                type_map = {}
                for full_decl, param_name in param_decls:
                    type_map[param_name] = full_decl
                ansi_params = []
                for pn in param_names:
                    if pn == '...':
                        ansi_params.append('...')
                    elif pn in type_map:
                        ansi_params.append(type_map[pn])
                    else:
                        ansi_params.append('int ' + pn)
                ansi_str = ', '.join(ansi_params) if ansi_params else ''
                if not params_str:
                    new_line = indent + funcname + '(void)\n'
                else:
                    new_line = indent + funcname + '(' + ansi_str + ')\n'
                modified.append(new_line)
                changes += 1
                i = j - 1
            else:
                modified.append(line)
        else:
            modified.append(line)
        i += 1
    if changes > 0:
        with open(filepath, 'w') as f:
            f.writelines(modified)
        print("  %s: %d K&R functions converted" % (filepath, changes))
    return changes

if __name__ == '__main__':
    total = 0
    for f in sys.argv[1:]:
        try:
            total += convert_file(f)
        except Exception as e:
            print("  ERROR in %s: %s" % (f, e))
    print("Total: %d functions converted" % total)

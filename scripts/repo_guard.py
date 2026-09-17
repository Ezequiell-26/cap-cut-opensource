#!/usr/bin/env python3
"""
CCOS Repository Guard - Seguridad y Validación de Commits
"""
import os, sys, re, json
from pathlib import Path
from typing import List

REPO_ROOT = Path(__file__).parent.parent
MAX_FILE_SIZE = 10 * 1024 * 1024
SECRET_PATTERNS = [
    r'(?i)(api[_-]?key|apikey)\s*[:=]\s*["\'][a-zA-Z0-9]{20,}["\']',
    r'(?i)(password|passwd|pwd)\s*[:=]\s*["\'][^"\']+["\']',
    r'(?i)(secret|token)\s*[:=]\s*["\'][a-zA-Z0-9]{16,}["\']',
    r'ghp_[a-zA-Z0-9]{36}',
    r'-----BEGIN.*PRIVATE KEY-----',
]
DANGEROUS_PATTERNS = [r'waitForFinished\s*\(\s*-1\s*\)', r'system\s*\([^)]*\+']

class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def check_secrets(files: List[Path]) -> List[str]:
    errors = []
    for f in files:
        if not f.exists() or f.suffix in ['.png','.jpg','.jpeg','.gif','.bin']: continue
        try:
            content = f.read_text(encoding='utf-8', errors='ignore')
            for p in SECRET_PATTERNS:
                if re.search(p, content):
                    errors.append(f"Secreto detectado en {f.relative_to(REPO_ROOT)}")
                    break
        except: pass
    return errors

def check_dangerous_patterns(files: List[Path]) -> List[str]:
    errors = []
    for f in files:
        if not f.exists() or f.suffix not in ['.h','.hpp','.c','.cpp']: continue
        try:
            content = f.read_text(encoding='utf-8', errors='ignore')
            for p in DANGEROUS_PATTERNS:
                if re.search(p, content):
                    errors.append(f"Patron peligroso en {f.relative_to(REPO_ROOT)}")
                    break
        except: pass
    return errors

def check_merge_conflicts(files: List[Path]) -> List[str]:
    errors = []
    for f in files:
        if not f.exists(): continue
        try:
            content = f.read_text(encoding='utf-8', errors='ignore')
            if '<<<<<<< ' in content or '======= ' in content or '>>>>>>> ' in content:
                errors.append(f"Conflicto en {f.relative_to(REPO_ROOT)}")
        except: pass
    return errors

def get_staged_files() -> List[Path]:
    import subprocess
    try:
        result = subprocess.run(['git', 'diff', '--cached', '--name-only'], cwd=REPO_ROOT, capture_output=True, text=True, timeout=10)
        if result.returncode != 0: return []
        return [REPO_ROOT / line for line in result.stdout.strip().split('\n') if line]
    except: return []

def main():
    print(f"{Colors.BOLD}CCOS Repository Guard{Colors.RESET}")
    print("=" * 50)
    files = get_staged_files()
    if not files:
        print(f"{Colors.BLUE}[INFO]{Colors.RESET} No hay archivos staged")
        return 0
    
    all_errors = []
    checks = [
        ("Secretos", lambda: check_secrets(files), True),
        ("Patrones peligrosos", lambda: check_dangerous_patterns(files), True),
        ("Conflictos", lambda: check_merge_conflicts(files), True),
    ]
    
    for name, func, critical in checks:
        print(f"Checking {name}...", end=" ")
        issues = func()
        if issues:
            all_errors.extend(issues)
            print(f"{Colors.RED}FAIL{Colors.RESET}")
        else:
            print(f"{Colors.GREEN}PASS{Colors.RESET}")
    
    print("=" * 50)
    if all_errors:
        for e in all_errors: print(f"  {Colors.RED}❌{Colors.RESET} {e}")
        print(f"\n{Colors.RED}Repo Guard FAILED{Colors.RESET}")
        return 1
    print(f"\n{Colors.GREEN}✓ Repo Guard PASSED{Colors.RESET}")
    return 0

if __name__ == '__main__':
    sys.exit(main())

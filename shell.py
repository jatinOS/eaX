#!/usr/bin/env python3
"""
eaX OS Shell jatiN corporation 2026
shell made by jatin labs
Supports Fortran, COBOL, ASM, C, H, CPP, D, M file execution
"""

import os
import sys
import subprocess
import json
import hashlib
import re
import shlex
import socket
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional

# eaX OS Shell Configuration
SHELL_VERSION = "1.0.0"
SHELL_NAME = "eaX Shell"
SHELL_PROMPT = "\033[36meaX>\033[0m "

# Supported programming languages and their compilers/interpreters
LANGUAGES = {
    '.f': {'name': 'Fortran', 'compiler': 'gfortran', 'run_flag': ''},
    '.f90': {'name': 'Fortran', 'compiler': 'gfortran', 'run_flag': ''},
    '.cbl': {'name': 'COBOL', 'compiler': 'cobc', 'run_flag': '-x'},
    '.cob': {'name': 'COBOL', 'compiler': 'cobc', 'run_flag': '-x'},
    '.asm': {'name': 'Assembly', 'compiler': 'nasm', 'run_flag': '-f elf64'},
    '.s': {'name': 'Assembly', 'compiler': 'as', 'run_flag': ''},
    '.c': {'name': 'C', 'compiler': 'gcc', 'run_flag': '-o'},
    '.h': {'name': 'C Header', 'compiler': 'gcc', 'run_flag': '-c'},
    '.cpp': {'name': 'C++', 'compiler': 'g++', 'run_flag': '-o'},
    '.cc': {'name': 'C++', 'compiler': 'g++', 'run_flag': '-o'},
    '.adb': {'name': 'Ada', 'compiler': 'gnatmake', 'run_flag': '-o'},
    '.ads': {'name': 'Ada', 'compiler': 'gnatmake', 'run_flag': '-o'},
    '.d': {'name': 'D', 'compiler': 'gdmd', 'run_flag': '-of'},
    '.m': {'name': 'Modula-2', 'compiler': 'gm2', 'run_flag': '-o'},
    '.py': {'name': 'Python', 'compiler': 'python3', 'run_flag': ''},
    '.sh': {'name': 'Shell', 'compiler': 'bash', 'run_flag': ''},
    '.bin': {'name': 'Binary', 'compiler': None, 'run_flag': None},
}

# File system simulation
class VirtualFileSystem:
    """Simulated file system for eaX OS"""
    
    def __init__(self):
        self.drives = {
            'C:': {'type': 'primary', 'size': '1TB', 'mount': '/', 'fs': 'eaXFS'},
            'A:': {'type': 'removable', 'size': '1.44MB', 'mount': '/mnt/a', 'fs': 'FAT12'},
            'B:': {'type': 'removable', 'size': 'variable', 'mount': '/downloads', 'fs': 'eaXFS'},
        }
        self.current_drive = 'C:'
        self.current_path = '/'
        
        # Simulated directory structure
        self.directories = {
            'C:/': {
                'type': 'dir',
                'children': ['bin', 'etc', 'home', 'tmp', 'dev', 'sys', 'proc', 'system', 'programs', 'games']
            },
            'C:/bin': {
                'type': 'dir',
                'children': ['shell.py', 'fortress.py', 'ls', 'cd', 'cat', 'mkdir', 'rm', 'cp', 'mv']
            },
            'C:/etc': {
                'type': 'dir',
                'children': ['system.cfg', 'fortress.cfg', 'passwd', 'group']
            },
            'C:/home': {
                'type': 'dir',
                'children': ['admin', 'user']
            },
            'C:/home/admin': {
                'type': 'dir',
                'children': ['documents', 'downloads', 'desktop', 'pictures', 'music']
            },
            'C:/system': {
                'type': 'dir',
                'children': ['kernel.bin', 'boot.bin', 'quarantine', 'drivers', 'modules']
            },
            'C:/programs': {
                'type': 'dir',
                'children': ['source_code', 'compiled', 'projects']
            },
            'C:/games': {
                'type': 'dir',
                'children': ['tictactoe', 'chess', 'checkers']
            },
        }
        
        # Simulated files with content
        self.files = {
            'C:/etc/system.cfg': self._load_system_cfg(),
            'C:/home/admin/documents/readme.txt': 'Welcome to eaX OS!\nThis is your personal computer.',
        }
    
    def _load_system_cfg(self):
        """Load system configuration"""
        return """# eaX OS System Configuration
[system]
name = eaX OS
version = 1.0.0
codename = Fortress
"""
    
    def list_directory(self, path: str = None) -> List[str]:
        """List directory contents"""
        if path is None:
            path = f"{self.current_drive}{self.current_path}"
        
        if path in self.directories:
            return self.directories[path]['children']
        return []
    
    def change_directory(self, path: str) -> bool:
        """Change current directory"""
        if path == '..':
            if self.current_path != '/':
                self.current_path = '/'.join(self.current_path.strip('/').split('/')[:-1])
                if not self.current_path.startswith('/'):
                    self.current_path = '/' + self.current_path
            return True
        elif path == '/':
            self.current_path = '/'
            return True
        elif path == '.':
            return True
        else:
            full_path = f"{self.current_drive}{self.current_path}{path}"
            if full_path in self.directories:
                self.current_path = self.current_path.rstrip('/') + '/' + path
                return True
            return False
    
    def get_file_content(self, path: str) -> Optional[str]:
        """Get file content"""
        full_path = f"{self.current_drive}{self.current_path}{path}"
        return self.files.get(full_path, None)
    
    def create_file(self, path: str, content: str = "") -> bool:
        """Create a new file"""
        full_path = f"{self.current_drive}{self.current_path}{path}"
        self.files[full_path] = content
        
        # Update parent directory
        parent_path = '/'.join(full_path.split('/')[:-1])
        if parent_path in self.directories:
            filename = full_path.split('/')[-1]
            if filename not in self.directories[parent_path]['children']:
                self.directories[parent_path]['children'].append(filename)
        
        return True
    
    def create_directory(self, path: str) -> bool:
        """Create a new directory"""
        full_path = f"{self.current_drive}{self.current_path}{path}"
        self.directories[full_path] = {'type': 'dir', 'children': []}
        
        # Update parent directory
        parent_path = '/'.join(full_path.split('/')[:-1])
        if parent_path in self.directories:
            if path not in self.directories[parent_path]['children']:
                self.directories[parent_path]['children'].append(path)
        
        return True


class eaXShell:
    """eaX OS Shell - Advanced terminal with programming support"""
    
    def __init__(self):
        self.version = SHELL_VERSION
        self.name = SHELL_NAME
        self.hostname = "eax-pc"
        self.username = "admin"
        self.home_dir = "/home/admin"
        self.current_dir = self.home_dir
        self.history = []
        self.history_index = -1
        self.vfs = VirtualFileSystem()
        self.running = True
        self.network_status = {
            'connected': False,
            'ssid': None,
            'interface': None,
        }
        
        # Colors
        self.COLORS = {
            'reset': '\033[0m',
            'red': '\033[31m',
            'green': '\033[32m',
            'yellow': '\033[33m',
            'blue': '\033[34m',
            'cyan': '\033[36m',
            'white': '\033[37m',
            'bold': '\033[1m',
        }
    
    def get_prompt(self) -> str:
        """Generate shell prompt"""
        path_display = self.current_dir.replace(self.home_dir, '~')
        return f"{self.COLORS['cyan']}{self.username}@{self.hostname}{self.COLORS['reset']}:{self.COLORS['yellow']}{path_display}{self.COLORS['reset']}$ "
    
    def print_colored(self, text: str, color: str = 'white'):
        """Print colored text"""
        print(f"{self.COLORS.get(color, '')}{text}{self.COLORS['reset']}")
    
    def print_banner(self):
        """Print shell banner"""
        banner = f"""
{self.COLORS['cyan']}╔══════════════════════════════════════════╗{self.COLORS['reset']}
{self.COLORS['cyan']}║                                          ║{self.COLORS['reset']}
{self.COLORS['cyan']}║{self.COLORS['bold']}          eaX OS Shell.py v{self.version}           {self.COLORS['reset']}{self.COLORS['cyan']}║{self.COLORS['reset']}
{self.COLORS['cyan']}║          Codename: Fortress pro              ║{self.COLORS['reset']}
{self.COLORS['cyan']}║                                          ║{self.COLORS['reset']}
{self.COLORS['cyan']}╚══════════════════════════════════════════╝{self.COLORS['reset']}

Type {self.COLORS['yellow']}'help'{self.COLORS['reset']} for available commands.
Supported languages: {self.COLORS['green']}Fortran, COBOL, ASM, C, H, CPP, D, M, Python{self.COLORS['reset']}
"""
        print(banner)
    
    def execute_command(self, command: str) -> int:
        """Execute a shell command"""
        if not command.strip():
            return 0
        
        # Add to history
        self.history.append(command)
        if len(self.history) > 1000:
            self.history = self.history[-1000:]
        
        # Parse command
        parts = shlex.split(command)
        cmd = parts[0].lower()
        args = parts[1:] if len(parts) > 1 else []
        
        # Built-in commands
        builtins = {
            'help': self.cmd_help,
            'ls': self.cmd_ls,
            'dir': self.cmd_ls,
            'cd': self.cmd_cd,
            'pwd': self.cmd_pwd,
            'cat': self.cmd_cat,
            'echo': self.cmd_echo,
            'clear': self.cmd_clear,
            'cls': self.cmd_clear,
            'mkdir': self.cmd_mkdir,
            'rmdir': self.cmd_rmdir,
            'rm': self.cmd_rm,
            'cp': self.cmd_cp,
            'mv': self.cmd_mv,
            'touch': self.cmd_touch,
            'run': self.cmd_run,
            'compile': self.cmd_compile,
            'exec': self.cmd_exec,
            'exit': self.cmd_exit,
            'quit': self.cmd_exit,
            'history': self.cmd_history,
            'whoami': self.cmd_whoami,
            'hostname': self.cmd_hostname,
            'date': self.cmd_date,
            'time': self.cmd_date,
            'uname': self.cmd_uname,
            'neofetch': self.cmd_neofetch,
            'sysinfo': self.cmd_sysinfo,
            'ps': self.cmd_ps,
            'top': self.cmd_top,
            'df': self.cmd_df,
            'du': self.cmd_du,
            'man': self.cmd_man,
            'type': self.cmd_type,
            'which': self.cmd_which,
            'file': self.cmd_file,
            'hexdump': self.cmd_hexdump,
            'scan': self.cmd_scan,
            'security': self.cmd_security,
            'network': self.cmd_network,
            'wifi': self.cmd_wifi,
            'iconbrowser': self.cmd_iconbrowser,
            'mouse': self.cmd_mouse,
            'cursor': self.cmd_mouse,
            'drivers': self.cmd_drivers,
        }
        
        if cmd in builtins:
            return builtins[cmd](args)
        else:
            # Try to execute as external command
            return self.execute_external(cmd, args)
    
    def cmd_help(self, args):
        """Display help information"""
        help_text = f"""
{self.COLORS['bold']}eaX OS Shell Commands{self.COLORS['reset']}
{'='*50}

{self.COLORS['cyan']}File Operations:{self.COLORS['reset']}
  ls, dir        - List directory contents
  cd <path>      - Change directory
  pwd            - Print working directory
  cat <file>     - Display file content
  mkdir <dir>    - Create directory
  rmdir <dir>    - Remove directory
  rm <file>      - Remove file
  cp <src> <dst> - Copy file
  mv <src> <dst> - Move/rename file
  touch <file>   - Create empty file

{self.COLORS['cyan']}Programming:{self.COLORS['reset']}
  run <file>     - Compile and run source file
  compile <file> - Compile source file
  exec <binary>  - Execute binary file

{self.COLORS['cyan']}System:{self.COLORS['reset']}
  help           - Show this help
  clear, cls     - Clear screen
  history        - Show command history
  whoami         - Show current user
  hostname       - Show hostname
  date, time     - Show date/time
  uname          - Show system info
  neofetch       - Show system overview
  sysinfo        - Show detailed system info
  ps             - Show running processes
  top            - Show system resources
  df             - Show disk space
  du             - Show directory size

{self.COLORS['cyan']}Security:{self.COLORS['reset']}
  scan <file>    - Scan file for malware
  security       - Show security status

{self.COLORS['cyan']}Network:{self.COLORS['reset']}
  network        - Show network status and interfaces
  wifi list      - Scan nearby Wi-Fi networks
  wifi status    - Show current Wi-Fi connection
  wifi connect <ssid> <password> - Connect to Wi-Fi

{self.COLORS['cyan']}Desktop:{self.COLORS['reset']}
  iconbrowser    - Open the icon browser
  mouse          - Simulate pointer movement controls
  cursor         - Alias for mouse
  drivers        - Show loaded drivers and status

{self.COLORS['cyan']}Other:{self.COLORS['reset']}
  man <cmd>      - Show manual for command
  type <cmd>     - Show command type
  which <cmd>    - Find command path
  file <file>    - Determine file type
  hexdump <file> - Show hex dump of file
  echo <text>    - Print text
  exit, quit     - Exit shell
"""
        print(help_text)
        return 0
    
    def cmd_ls(self, args):
        """List directory contents"""
        path = args[0] if args else self.current_dir
        
        try:
            items = self.vfs.list_directory(path)
            if items:
                for item in sorted(items):
                    # Check if it's a directory
                    full_path = f"{path.rstrip('/')}/{item}"
                    if full_path in self.vfs.directories:
                        self.print_colored(f"  📁 {item}/", 'blue')
                    else:
                        # Determine file type by extension
                        ext = item.split('.')[-1] if '.' in item else ''
                        if ext in ['c', 'h', 'cpp', 'f', 'f90', 'cbl', 'asm', 'd', 'm', 'py']:
                            self.print_colored(f"  📄 {item}", 'green')
                        elif ext in ['exe', 'bin', 'dll', 'sys']:
                            self.print_colored(f"  ⚙️  {item}", 'yellow')
                        else:
                            print(f"  📄 {item}")
            else:
                print("  Directory is empty")
        except Exception as e:
            self.print_colored(f"  Error: {e}", 'red')
        
        return 0
    
    def cmd_cd(self, args):
        """Change directory"""
        if not args:
            self.current_dir = self.home_dir
            return 0
        
        path = args[0]
        if self.vfs.change_directory(path):
            self.current_dir = f"{self.vfs.current_drive}{self.vfs.current_path}"
        else:
            self.print_colored(f"  cd: {path}: No such directory", 'red')
            return 1
        
        return 0
    
    def cmd_pwd(self, args):
        """Print working directory"""
        print(f"  {self.current_dir}")
        return 0
    
    def cmd_cat(self, args):
        """Display file content"""
        if not args:
            print("  Usage: cat <file>")
            return 1
        
        content = self.vfs.get_file_content(args[0])
        if content:
            print(content)
        else:
            self.print_colored(f"  cat: {args[0]}: File not found", 'red')
            return 1
        
        return 0
    
    def cmd_echo(self, args):
        """Print text"""
        print(' '.join(args))
        return 0
    
    def cmd_clear(self, args):
        """Clear screen"""
        os.system('clear' if os.name != 'nt' else 'cls')
        return 0
    
    def cmd_mkdir(self, args):
        """Create directory"""
        if not args:
            print("  Usage: mkdir <directory>")
            return 1
        
        if self.vfs.create_directory(args[0]):
            self.print_colored(f"  Created directory: {args[0]}", 'green')
        else:
            self.print_colored(f"  mkdir: Cannot create directory", 'red')
            return 1
        
        return 0
    
    def cmd_rmdir(self, args):
        """Remove directory"""
        if not args:
            print("  Usage: rmdir <directory>")
            return 1
        
        print(f"  Removed directory: {args[0]}")
        return 0
    
    def cmd_rm(self, args):
        """Remove file"""
        if not args:
            print("  Usage: rm <file>")
            return 1
        
        print(f"  Removed file: {args[0]}")
        return 0
    
    def cmd_cp(self, args):
        """Copy file"""
        if len(args) < 2:
            print("  Usage: cp <source> <destination>")
            return 1
        
        print(f"  Copied {args[0]} to {args[1]}")
        return 0
    
    def cmd_mv(self, args):
        """Move/rename file"""
        if len(args) < 2:
            print("  Usage: mv <source> <destination>")
            return 1
        
        print(f"  Moved {args[0]} to {args[1]}")
        return 0
    
    def cmd_touch(self, args):
        """Create empty file"""
        if not args:
            print("  Usage: touch <file>")
            return 1
        
        self.vfs.create_file(args[0], "")
        self.print_colored(f"  Created file: {args[0]}", 'green')
        return 0
    
    def cmd_run(self, args):
        """Compile and run source file"""
        if not args:
            print("  Usage: run <source_file>")
            return 1
        
        filename = args[0]
        ext = '.' + filename.split('.')[-1] if '.' in filename else ''
        
        if ext not in LANGUAGES:
            self.print_colored(f"  Unsupported file type: {ext}", 'red')
            return 1
        
        lang = LANGUAGES[ext]
        self.print_colored(f"\n  Compiling {lang['name']} source: {filename}", 'cyan')
        
        # Simulate compilation
        if lang['compiler']:
            compile_cmd = f"{lang['compiler']} {lang['run_flag']} {filename}"
            self.print_colored(f"  > {compile_cmd}", 'yellow')
            
            # In real implementation, would call subprocess
            print(f"  [{lang['name']}] Compilation successful!")
        
        # Simulate execution
        output_file = filename.rsplit('.', 1)[0]
        self.print_colored(f"\n  Running: ./{output_file}", 'green')
        print(f"  [{lang['name']}] Program executed successfully!")
        
        return 0
    
    def cmd_compile(self, args):
        """Compile source file"""
        if not args:
            print("  Usage: compile <source_file>")
            return 1
        
        filename = args[0]
        ext = '.' + filename.split('.')[-1] if '.' in filename else ''
        
        if ext not in LANGUAGES:
            self.print_colored(f"  Unsupported file type: {ext}", 'red')
            return 1
        
        lang = LANGUAGES[ext]
        self.print_colored(f"\n  Compiling {lang['name']} source: {filename}", 'cyan')
        
        if lang['compiler']:
            compile_cmd = f"{lang['compiler']} {lang['run_flag']} {filename}"
            self.print_colored(f"  > {compile_cmd}", 'yellow')
            print(f"  [{lang['name']}] Compilation successful!")
        else:
            print(f"  [{lang['name']}] File is already compiled.")
        
        return 0
    
    def cmd_exec(self, args):
        """Execute binary file"""
        if not args:
            print("  Usage: exec <binary_file>")
            return 1
        
        filename = args[0]
        self.print_colored(f"\n  Executing: {filename}", 'green')
        print(f"  [Binary] Program executed successfully!")
        return 0
    
    def cmd_exit(self, args):
        """Exit shell"""
        self.print_colored("\n  Goodbye from eaX OS Shell!", 'cyan')
        self.running = False
        return 0
    
    def cmd_history(self, args):
        """Show command history"""
        for i, cmd in enumerate(self.history[-50:], 1):
            print(f"  {i:3d}  {cmd}")
        return 0
    
    def cmd_whoami(self, args):
        """Show current user"""
        print(f"  {self.username}")
        return 0
    
    def cmd_hostname(self, args):
        """Show hostname"""
        print(f"  {self.hostname}")
        return 0
    
    def cmd_date(self, args):
        """Show date/time"""
        now = datetime.now()
        print(f"  {now.strftime('%A, %B %d, %Y %H:%M:%S')}")
        return 0
    
    def cmd_uname(self, args):
        """Show system info"""
        if args and '-a' in args:
            print(f"  eaX OS {self.hostname} 1.0.0 Fortress x86_64 eaX")
        else:
            print(f"  eaX OS")
        return 0
    
    def cmd_neofetch(self, args):
        """Show system overview"""
        neofetch = f"""
{self.COLORS['cyan']}        ╔══════════════════════════════════════╗{self.COLORS['reset']}
{self.COLORS['cyan']}        ║                  eaX OS                ║{self.COLORS['reset']}
{self.COLORS['cyan']}        ╚══════════════════════════════════════╝{self.COLORS['reset']}

{self.COLORS['bold']}OS:{self.COLORS['reset']}     eaX OS 1.0.0 (Fortress)
{self.COLORS['bold']}Host:{self.COLORS['reset']}    eaX-PC x86_64
{self.COLORS['bold']}Kernel:{self.COLORS['reset']}  eax_kernel 1.0.0
{self.COLORS['bold']}Uptime:{self.COLORS['reset']}  0 days, 0 hours, 0 minutes
{self.COLORS['bold']}Shell:{self.COLORS['reset']}   eaX Shell {self.version}
{self.COLORS['bold']}Terminal:{self.COLORS['reset']} eaX Terminal
{self.COLORS['bold']}CPU:{self.COLORS['reset']}     Intel Core i9-13900K @ 5.8GHz
{self.COLORS['bold']}GPU:{self.COLORS['reset']}     eaX Graphics 164 MB
{self.COLORS['bold']}Memory:{self.COLORS['reset']}  16384 MiB / 65536 MiB (25%)
{self.COLORS['bold']}Disk:{self.COLORS['reset']}    189 GiB / 189 GiB (50%)

{self.COLORS['bold']}Security:{self.COLORS['reset']} Fotress AI - ACTIVE
{self.COLORS['bold']}Theme:{self.COLORS['reset']}   eaX Dark
"""
        print(neofetch)
        return 0
    
    def cmd_sysinfo(self, args):
        """Show detailed system info"""
        sysinfo = f"""
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}
{self.COLORS['bold']}           eaX OS System Information{self.COLORS['reset']}
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}

{self.COLORS['cyan']}System:{self.COLORS['reset']}
  OS Name:         eaX OS
  Version:         1.0.0
  Codename:        Fortress
  Architecture:    x86_64
  Kernel:          eax_kernel 1.0.0 (modular w/ drivers)
  Build Type:      Release

{self.COLORS['cyan']}Hardware:{self.COLORS['reset']}
  CPU:             Intel Core i9-13900K
  CPU Speed:       5.8 GHz
  Cores:           24 (8P + 16E)
  Threads:         32
  RAM:             64GB DDRS
  GPU:             eaX Graphics 164 MB
  Storage:         189 GB NVMe SSD
  Compatibility:    Intel/AMD x86_64, Mac virtualization

{self.COLORS['cyan']}Storage Subsystem:{self.COLORS['reset']}
  Primary Drive:   189 GB (NVMe, 4K sectors)
  Secondary Drive: 300 GB (SSD, 4K sectors)
  Floppy Drive:    1.44 MB (A: disabled)
  File System:     eaXFS with journaling
  Cache:           1 MB L3

{self.COLORS['cyan']}Network Subsystem:{self.COLORS['reset']}
  Ethernet:        Gigabit (1000 Mbps)
  Wi-Fi:           802.11ac (Wi-Fi 5)
  IPv4 Stack:      Active
  IPv6 Stack:      Supported
  MAC Address:     52:54:00:12:34:56

{self.COLORS['cyan']}Graphics Subsystem:{self.COLORS['reset']}
  GPU Memory:      164 MB VRAM
  Texture Cache:   50 MB
  Resolution:      1920x1080 @ 60Hz
  Color Depth:     32-bit RGBA
  V-Sync:          Enabled
  Hardware Accel:  Yes

{self.COLORS['cyan']}Input Subsystem:{self.COLORS['reset']}
  Keyboard:        Connected (PS/2)
  Mouse:           Connected (PS/2)
  Acceleration:    Level 1
  Events Queued:   0

{self.COLORS['cyan']}Software:{self.COLORS['reset']}
  Shell:           eaX Shell {self.version}
  Window System:   eaX Desktop Environment
  File System:     eaXFS
  Security:        Fotress AI v1.0

{self.COLORS['cyan']}Supported Languages:{self.COLORS['reset']}
  Fortran, COBOL, Assembly (ASM), Ada
  C, C++, D, Modula-2
  Python, Shell Script

{self.COLORS['cyan']}Security Status:{self.COLORS['reset']}
  Level:           FORTRESS
  AI Protection:   ACTIVE
  Malware Scan:    ENABLED
  Real-time Guard: ON
  DLL Protection:  ACTIVE
  EXE Protection:  ACTIVE
"""
        print(sysinfo)
        return 0
    
    def cmd_ps(self, args):
        """Show running processes"""
        processes = f"""
{self.COLORS['bold']}PID    USER     CPU%   MEM%   COMMAND{self.COLORS['reset']}
  1      root      0.0    0.1   /sbin/init
  2      root      0.0    0.0   [kthread]
  3      root      0.0    0.0   [kworker/0:0]
  4      root      0.0    0.0   [kworker/u32:0]
128     admin     2.5    1.2   eaX Desktop
256     admin     0.5    0.8   eaX Shell
512     admin     0.2    0.5   Fotress AI
1024    admin     0.1    0.3   System Monitor
"""
        print(processes)
        return 0
    
    def cmd_top(self, args):
        """Show system resources"""
        top = f"""
{self.COLORS['bold']}top - {datetime.now().strftime('%H:%M:%S')} up 0:05,  1 user,  load average: 0.05, 0.10, 0.05{self.COLORS['reset']}
{self.COLORS['bold']}Tasks: 8 total, 1 running, 7 sleeping{self.COLORS['reset']}
{self.COLORS['bold']}%Cpu(s):  2.5 us,  1.0 sy,  0.0 ni, 96.5 id{self.COLORS['reset']}
{self.COLORS['bold']}MiB Mem :  65536 total,  49152 free,  10240 used,  6144 buff/cache{self.COLORS['reset']}
{self.COLORS['bold']}MiB Swap:  16384 total,  16384 free,      0 used.{self.COLORS['reset']}

{self.COLORS['cyan']}  PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND{self.COLORS['reset']}
    256 admin     20   0   15000  12000   8000 S   2.5   0.8   0:05.00 eaX Shell
    512 admin     20   0   25000  20000  12000 S   0.5   0.5   0:02.00 Fotress AI
    128 admin     20   0   50000  40000  20000 S   0.2   1.2   0:10.00 eaX Desktop
"""
        print(top)
        return 0
    
    def cmd_df(self, args):
        """Show disk space"""
        df = f"""
{self.COLORS['bold']}Filesystem      Size  Used Avail Use% Mounted on{self.COLORS['reset']}
C:              1000G  500G  500G  50% /
A:               1.4M     0  1.4M   0% /mnt/a
B:               100G   20G   80G  20% /downloads
"""
        print(df)
        return 0
    
    def cmd_du(self, args):
        """Show directory size"""
        path = args[0] if args else '.'
        print(f"  {path}: 500G")
        return 0
    
    def cmd_man(self, args):
        """Show manual"""
        if not args:
            print("  Usage: man <command>")
            return 1
        
        cmd = args[0]
        print(f"  No manual entry for '{cmd}' in eaX OS yet.")
        return 0
    
    def cmd_type(self, args):
        """Show command type"""
        if not args:
            print("  Usage: type <command>")
            return 1
        
        cmd = args[0]
        builtins = ['help', 'ls', 'cd', 'pwd', 'cat', 'echo', 'clear', 'mkdir', 'rm', 'cp', 'mv']
        if cmd in builtins:
            print(f"  {cmd} is a shell builtin")
        else:
            print(f"  {cmd} is an external command")
        return 0
    
    def cmd_which(self, args):
        """Find command path"""
        if not args:
            print("  Usage: which <command>")
            return 1
        
        print(f"  /bin/{args[0]}")
        return 0
    
    def cmd_file(self, args):
        """Determine file type"""
        if not args:
            print("  Usage: file <file>")
            return 1
        
        filename = args[0]
        ext = filename.split('.')[-1] if '.' in filename else 'data'
        
        file_types = {
            'c': 'C source code',
            'h': 'C header file',
            'cpp': 'C++ source code',
            'f': 'Fortran source code',
            'f90': 'Fortran 90 source code',
            'cbl': 'COBOL source code',
            'asm': 'Assembly source code',
            'd': 'D source code',
            'm': 'Modula-2 source code',
            'py': 'Python script',
            'sh': 'Shell script',
            'bin': 'Binary executable',
            'exe': 'DOS/Windows executable',
            'dll': 'Dynamic link library',
        }
        
        ftype = file_types.get(ext, f'{ext.upper()} file')
        print(f"  {filename}: {ftype}")
        return 0
    
    def cmd_hexdump(self, args):
        """Show hex dump of file"""
        if not args:
            print("  Usage: hexdump <file>")
            return 1
        
        filename = args[0]
        print(f"  Hex dump of {filename}:")
        print(f"  {'='*50}")
        
        # Simulated hex dump
        hex_data = [
            "00000000  4d 5a 90 00 03 00 00 00  04 00 00 00 ff ff 00 00  |MZ..............|",
            "00000010  b8 00 00 00 00 00 00 00  40 00 00 00 00 00 00 00  |........@.......|",
            "00000020  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|",
            "00000030  00 00 00 00 00 00 00 00  00 00 00 00 80 00 00 00  |................|",
            "00000040  0e 1f ba 0e 00 b4 09 cd  21 b8 01 4c cd 21 54 68  |........!..L.!Th|",
            "00000050  69 73 20 70 72 6f 67 72  61 6d 20 63 61 6e 6e 6f  |is program canno|",
            "00000060  74 20 62 65 20 72 75 6e  20 69 6e 20 44 4f 53 20  |t be run in Dos |",
            "00000070  6d 6f 64 65 2e 0d 0d 0a  24 00 00 00 00 00 00 00  |mode....$.......|",
        ]
        
        for line in hex_data:
            print(f"  {line}")
        
        return 0
    
    def cmd_scan(self, args):
        """Scan file for malware"""
        if not args:
            print("  Usage: scan <file>")
            return 1
        
        filename = args[0]
        self.print_colored(f"\n  Scanning: {filename}", 'cyan')
        print("  [Fotress AI] Analyzing file...")
        
        # Simulate scan
        import time
        time.sleep(1)
        
        self.print_colored("  ✓ File is CLEAN - No threats detected", 'green')
        print("  [Fotress AI] File passed all security checks.")
        return 0
    
    def cmd_security(self, args):
        """Show security status"""
        security = f"""
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}
{self.COLORS['bold']}         eaX Security Status (Fotress AI){self.COLORS['reset']}
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}

{self.COLORS['green']}● Security Level: FORTRESS{self.COLORS['reset']}
{self.COLORS['green']}● AI Protection: ACTIVE{self.COLORS['reset']}
{self.COLORS['green']}● Real-time Scanning: ON{self.COLORS['reset']}
{self.COLORS['green']}● Malware Detection: ENABLED{self.COLORS['reset']}
{self.COLORS['green']}● DLL Protection: ACTIVE{self.COLORS['reset']}
{self.COLORS['green']}● EXE Protection: ACTIVE{self.COLORS['reset']}
{self.COLORS['green']}● File Monitoring: ON{self.COLORS['reset']}

{self.COLORS['cyan']}Statistics:{self.COLORS['reset']}
  Files Scanned:      1,234
  Threats Detected:   0
  Files Quarantined:  0
  Last Scan:          Just now

{self.COLORS['green']}System is SECURE{self.COLORS['reset']}
"""
        print(security)
        return 0

    def cmd_network(self, args):
        if args and args[0] == 'status':
            self.print_colored('  Network status', 'cyan')
            self.print_colored(f"  Interface: {self.network_status.get('interface') or 'default'}", 'yellow')
            self.print_colored(f"  Connected: {'Yes' if self.network_status['connected'] else 'No'}", 'green' if self.network_status['connected'] else 'red')
            self.print_colored(f"  SSID: {self.network_status.get('ssid') or 'None'}", 'cyan')
            return 0

        print('  Available network interfaces:')
        print('    - eaX Ethernet')
        print('    - eaX Wi-Fi 164MB')
        print('    - Host bridge support for Intel/AMD/Mac virtualization')
        return 0

    def cmd_wifi(self, args):
        if not args:
            return self.wifi_list()

        action = args[0].lower()
        if action in ('list', 'scan'):
            return self.wifi_list()

        if action == 'status':
            return self.wifi_status()

        if action == 'connect':
            if len(args) < 3:
                print('  Usage: wifi connect <ssid> <password>')
                return 1

            ssid = args[1]
            password = ' '.join(args[2:])
            return self.wifi_connect(ssid, password)

        print('  Usage: wifi list | wifi status | wifi connect <ssid> <password>')
        return 1

    def wifi_list(self):
        print('  Scanning Wi-Fi networks...')
        lines = []

        if sys.platform.startswith('win'):
            lines = self.run_cmd(['netsh', 'wlan', 'show', 'networks', 'mode=bssid'])
        elif sys.platform.startswith('linux'):
            lines = self.run_cmd(['nmcli', '-t', '-f', 'SSID,BSSID,CHAN,SIGNAL', 'device', 'wifi', 'list'])
        elif sys.platform.startswith('darwin'):
            lines = self.run_cmd(['/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport', '-s'])

        if lines:
            for line in lines[:8]:
                print(f'  {line}')
            print('  …')
            return 0

        print('  No host wifi scan tool available, showing simulated networks:')
        print('    1. eaX-Home-164MB   Location: Living Room')
        print('    2. FortressGuest    Location: Lobby')
        print('    3. MacNet-Global    Location: Apple Park')
        return 0

    def wifi_status(self):
        print('  Wi-Fi connection status')
        print(f"    Connected: {'Yes' if self.network_status['connected'] else 'No'}")
        print(f"    SSID: {self.network_status.get('ssid') or 'None'}")
        print(f"    Interface: {self.network_status.get('interface') or 'Wi-Fi'}")
        return 0

    def wifi_connect(self, ssid, password):
        if sys.platform.startswith('win'):
            result = self.wifi_connect_windows(ssid, password)
        elif sys.platform.startswith('linux'):
            result = self.wifi_connect_linux(ssid, password)
        elif sys.platform.startswith('darwin'):
            result = self.wifi_connect_macos(ssid, password)
        else:
            result = False

        if result:
            self.network_status.update({'connected': True, 'ssid': ssid, 'interface': 'Wi-Fi'})
            self.print_colored(f'  Connected to {ssid}', 'green')
            return 0

        self.print_colored('  Unable to connect using host tools, switching to simulated connection', 'yellow')
        self.network_status.update({'connected': True, 'ssid': ssid, 'interface': 'Wi-Fi'})
        print(f'  Connected to {ssid} (simulated)')
        return 0

    def wifi_connect_windows(self, ssid, password):
        if self.run_cmd(['netsh', 'wlan', 'connect', 'name=' + ssid, 'ssid=' + ssid]):
            return True
        return False

    def wifi_connect_linux(self, ssid, password):
        if self.run_cmd(['nmcli', 'device', 'wifi', 'connect', ssid, 'password', password]):
            return True
        return False

    def wifi_connect_macos(self, ssid, password):
        interface_lines = self.run_cmd(['networksetup', '-listallhardwareports'])
        airport = None

        for line in interface_lines:
            if 'Device:' in line and airport is None:
                airport = line.split(':')[-1].strip()

        if airport:
            if self.run_cmd(['networksetup', '-setairportnetwork', airport, ssid, password]):
                return True

        return False

    def cmd_iconbrowser(self, args):
        apps = [
            ('📝', 'Text Editor', 'editor'),
            ('🌐', 'Browser', 'browser'),
            ('🔒', 'Security', 'security'),
            ('📶', 'Network', 'network'),
            ('🖼️', 'Graphics', 'graphics'),
        ]

        print('  eaX Icon Browser')
        for i, item in enumerate(apps, 1):
            print(f'    {i}. {item[0]} {item[1]}')

        choice = input('  Enter icon number to open: ').strip()
        if not choice.isdigit() or not (1 <= int(choice) <= len(apps)):
            print('  Invalid selection')
            return 1

        index = int(choice) - 1
        name = apps[index][1]
        self.print_colored(f'  Opening {name}...', 'cyan')

        if apps[index][2] == 'security':
            return self.cmd_security([])
        if apps[index][2] == 'network':
            return self.cmd_network([])

        print('  Launching', name)
        return 0

    def cmd_mouse(self, args):
        x, y = 10, 10
        print('  Mouse simulator ready. Use w/a/s/d to move, q to quit.')

        while True:
            print(f'  Cursor position: {x},{y}')
            move = input('  Move (w/a/s/d/q): ').strip().lower()
            if move == 'q':
                break
            if move == 'w':
                y = max(0, y - 1)
            elif move == 's':
                y += 1
            elif move == 'a':
                x = max(0, x - 1)
            elif move == 'd':
                x += 1
            else:
                print('  Use w/a/s/d or q')

        print('  Mouse simulator closed')
        return 0

    def run_cmd(self, args):
        try:
            output = subprocess.check_output(args, stderr=subprocess.DEVNULL, text=True)
            return output.splitlines()
        except Exception:
            return []

    def cmd_drivers(self, args):
        drivers_info = f"""
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}
{self.COLORS['bold']}              Loaded Drivers{self.COLORS['reset']}
{self.COLORS['bold']}═══════════════════════════════════════════{self.COLORS['reset']}

{self.COLORS['green']}[OK]{self.COLORS['reset']} Storage Driver v1.0
  Devices:  3 drives configured
  Primary:  189 GB NVMe SSD (sda)
  Status:   Active

{self.COLORS['green']}[OK]{self.COLORS['reset']} Network Driver v1.0
  Interfaces: 2 configured
  Ethernet:   1000 Mbps (Active)
  Wi-Fi:      802.11ac (Enabled)
  Status:     Active

{self.COLORS['green']}[OK]{self.COLORS['reset']} Graphics Driver v1.0
  GPU Memory: 164 MB
  Mode:       1920x1080 @ 32-bit
  Textures:   10 loaded
  Status:     Active

{self.COLORS['green']}[OK]{self.COLORS['reset']} Input Driver v1.0
  Devices:    2 configured
  Keyboard:   PS/2 (Connected)
  Mouse:      PS/2 (Connected)
  Status:     Active
"""
        print(drivers_info)
        return 0

    def execute_external(self, cmd: str, args: List[str]) -> int:
        """Execute external command"""
        try:
            # In real implementation, would find and execute external commands
            self.print_colored(f"  Command not found: {cmd}", 'red')
            return 127
        except Exception as e:
            self.print_colored(f"  Error: {e}", 'red')
            return 1
    
    def run(self):
        """Main shell loop"""
        self.print_banner()
        
        while self.running:
            try:
                # Get command
                command = input(self.get_prompt()).strip()
                
                if command:
                    # Execute command
                    self.execute_command(command)
                    
            except KeyboardInterrupt:
                print()
                continue
            except EOFError:
                print()
                break
        
        self.cmd_exit([])


def main():
    """Main entry point"""
    shell = eaXShell()
    shell.run()


if __name__ == "__main__":
    main()

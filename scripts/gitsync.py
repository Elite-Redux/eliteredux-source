#!/usr/bin/env python3
"""
Smart Git Sync Tool for Elite Redux
Handles pull, commit, and push operations intelligently
"""

import subprocess
import sys
import os
import datetime
from typing import List, Tuple

class GitSync:
    def __init__(self):
        self.has_changes = False
        self.has_staged = False
        self.has_conflicts = False
        self.current_branch = ""
        self.upstream_branch = ""
        
    def run_command(self, cmd: List[str], check=True) -> Tuple[int, str, str]:
        """Run a command and return exit code, stdout, and stderr"""
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, check=check)
            return result.returncode, result.stdout.strip(), result.stderr.strip()
        except subprocess.CalledProcessError as e:
            return e.returncode, e.stdout.strip() if e.stdout else "", e.stderr.strip() if e.stderr else ""
    
    def check_status(self):
        """Check current git status"""
        print("📊 Checking git status...")
        
        # Get current branch
        _, branch_out, _ = self.run_command(["git", "branch", "--show-current"])
        self.current_branch = branch_out
        print(f"   Current branch: {self.current_branch}")
        
        # Check for uncommitted changes
        _, status_out, _ = self.run_command(["git", "status", "--porcelain"])
        if status_out:
            self.has_changes = True
            staged_files = []
            modified_files = []
            untracked_files = []
            
            for line in status_out.split('\n'):
                if line.startswith('A ') or line.startswith('M '):
                    self.has_staged = True
                    staged_files.append(line[3:])
                elif line.startswith(' M'):
                    modified_files.append(line[3:])
                elif line.startswith('??'):
                    untracked_files.append(line[3:])
            
            print(f"   Staged files: {len(staged_files)}")
            print(f"   Modified files: {len(modified_files)}")
            print(f"   Untracked files: {len(untracked_files)}")
        else:
            print("   Working directory is clean")
            
        # Check if we have upstream
        _, upstream_out, _ = self.run_command(["git", "rev-parse", "--abbrev-ref", f"{self.current_branch}@{{upstream}}"], check=False)
        if upstream_out:
            self.upstream_branch = upstream_out
            print(f"   Tracking: {self.upstream_branch}")
        else:
            print("   ⚠️  No upstream branch set")
    
    def stash_changes(self):
        """Stash uncommitted changes if any"""
        if self.has_changes and not self.has_staged:
            print("\n📦 Stashing uncommitted changes...")
            _, _, _ = self.run_command(["git", "stash", "push", "-m", f"Auto-stash before sync at {datetime.datetime.now()}"])
            return True
        return False
    
    def pull_changes(self):
        """Pull latest changes from remote"""
        print("\n⬇️  Pulling latest changes...")
        
        if not self.upstream_branch:
            print("   No upstream branch to pull from")
            return True
            
        code, out, err = self.run_command(["git", "pull", "--rebase"], check=False)
        
        if code == 0:
            if "Already up to date" in out:
                print("   Already up to date")
            else:
                print("   Successfully pulled and rebased changes")
            return True
        else:
            if "CONFLICT" in out or "CONFLICT" in err:
                print("   ❌ Merge conflicts detected!")
                self.has_conflicts = True
            else:
                print(f"   ❌ Pull failed: {err}")
            return False
    
    def apply_stash(self):
        """Apply stashed changes back"""
        print("\n📤 Applying stashed changes...")
        code, out, err = self.run_command(["git", "stash", "pop"], check=False)
        if code == 0:
            print("   Successfully applied stashed changes")
            return True
        else:
            print(f"   ⚠️  Failed to apply stash: {err}")
            print("   Run 'git stash pop' manually to recover changes")
            return False
    
    def smart_commit(self):
        """Create a smart commit based on changes"""
        if not self.has_changes:
            return True
            
        print("\n💾 Creating smart commit...")
        
        # Get list of changed files
        _, status_out, _ = self.run_command(["git", "status", "--porcelain"])
        
        # Stage all changes
        print("   Staging all changes...")
        self.run_command(["git", "add", "-A"])
        
        # Analyze changes to create commit message
        _, diff_stat, _ = self.run_command(["git", "diff", "--cached", "--stat"])
        _, diff_names, _ = self.run_command(["git", "diff", "--cached", "--name-only"])
        
        changed_files = diff_names.split('\n') if diff_names else []
        
        # Create smart commit message
        if len(changed_files) == 1:
            file_path = changed_files[0]
            file_name = os.path.basename(file_path)
            if file_path.startswith('src/'):
                message = f"Update {file_name}"
            elif file_path.startswith('data/'):
                message = f"Update game data: {file_name}"
            elif file_path.startswith('graphics/'):
                message = f"Update graphics: {file_name}"
            elif file_path.startswith('scripts/'):
                message = f"Add/update script: {file_name}"
            else:
                message = f"Update {file_name}"
        else:
            # Group by directory
            dirs = {}
            for f in changed_files:
                dir_name = f.split('/')[0] if '/' in f else 'root'
                dirs[dir_name] = dirs.get(dir_name, 0) + 1
            
            if len(dirs) == 1:
                dir_name = list(dirs.keys())[0]
                message = f"Update {dir_name} ({len(changed_files)} files)"
            else:
                # Find most common directory
                main_dir = max(dirs, key=dirs.get)
                message = f"Update {main_dir} and {len(dirs)-1} other areas"
        
        # Add timestamp
        message += f"\n\nAuto-sync at {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
        
        print(f"   Commit message: {message.split(chr(10))[0]}")
        
        # Create commit
        code, out, err = self.run_command(["git", "commit", "-m", message], check=False)
        if code == 0:
            print("   ✅ Successfully created commit")
            return True
        else:
            print(f"   ❌ Commit failed: {err}")
            return False
    
    def push_changes(self):
        """Push changes to remote"""
        print("\n⬆️  Pushing to remote...")
        
        if not self.upstream_branch:
            # Set upstream if not set
            print("   Setting upstream branch...")
            code, out, err = self.run_command(["git", "push", "-u", "origin", self.current_branch], check=False)
        else:
            code, out, err = self.run_command(["git", "push"], check=False)
        
        if code == 0:
            print("   ✅ Successfully pushed to remote")
            return True
        else:
            print(f"   ❌ Push failed: {err}")
            return False
    
    def sync(self):
        """Main sync operation"""
        print("🔄 Elite Redux Git Sync Tool")
        print("=" * 40)
        
        # Check status
        self.check_status()
        
        # Stash if needed
        stashed = self.stash_changes()
        
        # Pull latest
        if not self.pull_changes():
            if self.has_conflicts:
                print("\n❌ Cannot continue due to merge conflicts")
                print("   Please resolve conflicts manually")
                return False
        
        # Apply stash if we stashed
        if stashed:
            self.apply_stash()
        
        # Commit if there are changes
        if self.has_changes:
            if not self.smart_commit():
                return False
        
        # Push
        if not self.push_changes():
            return False
        
        print("\n✅ Sync completed successfully!")
        return True

def main():
    # Check if we're in a git repository
    if not os.path.exists('.git'):
        print("❌ Not in a git repository!")
        sys.exit(1)
    
    # Run sync
    syncer = GitSync()
    success = syncer.sync()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
#!/usr/bin/env python3
"""
Migration script for CPlayerController - replaces CGame member accesses
with CPlayer::m_Controller method calls.
"""
import os
import re

# Files to process
files = [
    r"Sources\Client\Game.cpp",
    r"Sources\Client\DialogBox_ItemDrop.cpp",
    r"Sources\Client\DialogBox_Skill.cpp",
    r"Sources\Client\DialogBox_HudPanel.cpp",
    r"Sources\Client\DialogBox_Magic.cpp",
    r"Sources\Client\DialogBox_Character.cpp",
    r"Sources\Client\NetworkMessages_Map.cpp",
    r"Sources\Client\NetworkMessages_Player.cpp",
    r"Sources\Client\NetworkMessages_Combat.cpp",
]

base_path = r"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82"

# Replacements for member variables accessed through different patterns
replacements = [
    # m_cCommand
    (r'm_cCommand\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetCommand(\1);'),
    (r'm_cCommand\s*==', r'm_pPlayer->m_Controller.GetCommand() =='),
    (r'm_cCommand\s*!=', r'm_pPlayer->m_Controller.GetCommand() !='),
    (r'\bm_cCommand\b(?!\s*[=])', r'm_pPlayer->m_Controller.GetCommand()'),

    # m_bCommandAvailable
    (r'm_bCommandAvailable\s*=\s*true\s*;', r'm_pPlayer->m_Controller.SetCommandAvailable(true);'),
    (r'm_bCommandAvailable\s*=\s*false\s*;', r'm_pPlayer->m_Controller.SetCommandAvailable(false);'),
    (r'm_bCommandAvailable\s*==\s*true', r'm_pPlayer->m_Controller.IsCommandAvailable() == true'),
    (r'm_bCommandAvailable\s*==\s*false', r'm_pPlayer->m_Controller.IsCommandAvailable() == false'),
    (r'\bm_bCommandAvailable\b(?!\s*[=])', r'm_pPlayer->m_Controller.IsCommandAvailable()'),

    # m_dwCommandTime
    (r'm_dwCommandTime\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetCommandTime(\1);'),
    (r'\bm_dwCommandTime\b(?!\s*[=])', r'm_pPlayer->m_Controller.GetCommandTime()'),

    # m_cCommandCount
    (r'm_cCommandCount\s*=\s*0\s*;', r'm_pPlayer->m_Controller.ResetCommandCount();'),
    (r'm_cCommandCount\s*\+\+', r'm_pPlayer->m_Controller.IncrementCommandCount()'),
    (r'm_cCommandCount\s*==', r'm_pPlayer->m_Controller.GetCommandCount() =='),
    (r'm_cCommandCount\s*>=', r'm_pPlayer->m_Controller.GetCommandCount() >='),
    (r'm_cCommandCount\s*<=', r'm_pPlayer->m_Controller.GetCommandCount() <='),
    (r'm_cCommandCount\s*>', r'm_pPlayer->m_Controller.GetCommandCount() >'),
    (r'm_cCommandCount\s*<', r'm_pPlayer->m_Controller.GetCommandCount() <'),
    (r'\bm_cCommandCount\b', r'm_pPlayer->m_Controller.GetCommandCount()'),

    # m_sCommX, m_sCommY
    (r'm_sCommX\s*=\s*([^;,]+)\s*;\s*m_sCommY\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetDestination(\1, \2);'),
    (r'm_sCommX\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetDestination(\1, m_pPlayer->m_Controller.GetDestinationY());'),
    (r'm_sCommY\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetDestination(m_pPlayer->m_Controller.GetDestinationX(), \1);'),
    (r'\bm_sCommX\b', r'm_pPlayer->m_Controller.GetDestinationX()'),
    (r'\bm_sCommY\b', r'm_pPlayer->m_Controller.GetDestinationY()'),

    # m_iPrevMoveX, m_iPrevMoveY
    (r'm_iPrevMoveX\s*=\s*([^;]+)\s*;\s*m_iPrevMoveY\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetPrevMove(\1, \2);'),
    (r'\bm_iPrevMoveX\b', r'm_pPlayer->m_Controller.GetPrevMoveX()'),
    (r'\bm_iPrevMoveY\b', r'm_pPlayer->m_Controller.GetPrevMoveY()'),

    # m_bIsPrevMoveBlocked
    (r'm_bIsPrevMoveBlocked\s*=\s*true\s*;', r'm_pPlayer->m_Controller.SetPrevMoveBlocked(true);'),
    (r'm_bIsPrevMoveBlocked\s*=\s*false\s*;', r'm_pPlayer->m_Controller.SetPrevMoveBlocked(false);'),
    (r'\bm_bIsPrevMoveBlocked\b', r'm_pPlayer->m_Controller.IsPrevMoveBlocked()'),

    # m_cPlayerTurn
    (r'm_cPlayerTurn\s*=\s*([^;]+);', r'm_pPlayer->m_Controller.SetPlayerTurn(\1);'),
    (r'\bm_cPlayerTurn\b', r'm_pPlayer->m_Controller.GetPlayerTurn()'),

    # cGetNextMoveDir function calls - replace with controller method
    (r'\bcGetNextMoveDir\s*\(\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*true,\s*true\s*\)',
     r'm_pPlayer->m_Controller.GetNextMoveDir(\1, \2, \3, \4, m_pMapData, true, true)'),
    (r'\bcGetNextMoveDir\s*\(\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*true\s*\)',
     r'm_pPlayer->m_Controller.GetNextMoveDir(\1, \2, \3, \4, m_pMapData, true)'),
    (r'\bcGetNextMoveDir\s*\(\s*([^,]+),\s*([^,]+),\s*([^,]+),\s*([^)]+)\)',
     r'm_pPlayer->m_Controller.GetNextMoveDir(\1, \2, \3, \4, m_pMapData)'),

    # GetPlayerTurn function calls
    (r'\bGetPlayerTurn\s*\(\s*\)\s*;', r'm_pPlayer->m_Controller.CalculatePlayerTurn(m_pPlayer->m_sPlayerX, m_pPlayer->m_sPlayerY, m_pMapData);'),
]

def process_file(filepath):
    print(f"Processing: {filepath}")

    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    original = content

    for pattern, replacement in replacements:
        content = re.sub(pattern, replacement, content)

    if content != original:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"  Modified: {filepath}")
    else:
        print(f"  No changes: {filepath}")

def main():
    for f in files:
        full_path = os.path.join(base_path, f)
        if os.path.exists(full_path):
            process_file(full_path)
        else:
            print(f"File not found: {full_path}")

    # Also check for any other files that might reference these variables
    client_dir = os.path.join(base_path, "Sources", "Client")
    for filename in os.listdir(client_dir):
        if filename.endswith('.cpp') and filename not in [os.path.basename(f) for f in files]:
            full_path = os.path.join(client_dir, filename)
            with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
            if any(x in content for x in ['m_cCommand', 'm_bCommandAvailable', 'm_dwCommandTime',
                                          'm_cCommandCount', 'm_sCommX', 'm_sCommY',
                                          'm_iPrevMoveX', 'm_iPrevMoveY', 'm_bIsPrevMoveBlocked',
                                          'm_cPlayerTurn', 'cGetNextMoveDir', 'GetPlayerTurn']):
                print(f"WARNING: {filename} may need manual updates")

if __name__ == '__main__':
    main()

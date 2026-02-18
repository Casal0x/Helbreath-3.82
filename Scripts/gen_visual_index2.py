import sqlite3, json, re
from collections import defaultdict

db = sqlite3.connect("Z:/Helbreath-3.82/Binaries/Server/gameconfigs.db")
cur = db.cursor()

equip_names = {
    0: "None", 1: "Head", 2: "Body", 3: "Arms", 4: "Pants",
    5: "Leggings", 6: "Neck", 7: "LeftHand", 8: "RightHand",
    9: "TwoHand", 10: "RightFinger", 11: "LeftFinger",
    12: "Back", 13: "FullBody",
}

ANIM_NAMES = [
    "standing_peace", "standing_combat", "walking_peace", "walking_combat",
    "running", "attack_standing", "attack_walk_1h", "attack_walk_2h",
    "magic_cast", "pickup", "take_damage", "dying",
]

# equipFrameMul per bodyPose
ARMOR_STRIDE = [8, 8, 8, 8, 8, 8, 8, 8, 16, 4, 4, 8]

# weaponPose mapping: bodyPose -> weaponPose (-1 = not drawn)
WEAPON_POSE = [0, 1, 2, 3, 6, -1, 4, 4, -1, -1, 5, -1]

# shieldPose mapping: bodyPose -> shieldPose (-1 = not drawn)
SHIELD_POSE = [0, 1, 2, 3, 6, -1, 4, 4, -1, -1, 5, -1]


def make_armor_animations():
    """All 12 bodyPoses valid, sprite N = bodyPose N in the PAK"""
    anims = {}
    for i, name in enumerate(ANIM_NAMES):
        anims[name] = {"pak_sprite": i, "stride": ARMOR_STRIDE[i]}
    return anims


def make_weapon_animations(first_sprite):
    """56 sprites per weapon: 8 * weaponPose + (dir-1). Null for poses where weapon isn't drawn."""
    anims = {}
    for i, name in enumerate(ANIM_NAMES):
        wp = WEAPON_POSE[i]
        if wp == -1:
            anims[name] = None
        else:
            anims[name] = {"pak_sprite": first_sprite + wp * 8, "stride": 8}
    return anims


def make_shield_animations(first_sprite):
    """7 sprites per shield: shieldPose 0-6. Null for poses where shield isn't drawn."""
    anims = {}
    for i, name in enumerate(ANIM_NAMES):
        sp = SHIELD_POSE[i]
        if sp == -1:
            anims[name] = None
        else:
            anims[name] = {"pak_sprite": first_sprite + sp, "stride": 8}
    return anims


# ---- PAK file mappings (from Screen_Loading.cpp) ----

body_m = {1:"mlarmor",2:"mcmail",3:"msmail",4:"mpmail",5:"mtunic",6:"mrobe1",7:"msanta",8:"mhpmail1",9:"mhpmail2",10:"mhrobe1",11:"mhrobe2"}
body_w = {1:"wbodice1",2:"wbodice2",3:"wlarmor",4:"wcmail",5:"wsmail",6:"wpmail",7:"wrobe1",8:"wsanta",9:"whpmail1",10:"whpmail2",11:"whrobe1",12:"whrobe2"}
arms_m = {1:"mshirt",2:"mhauberk",3:"mhhauberk1",4:"mhhauberk2"}
arms_w = {1:"wchemiss",2:"wshirt",3:"whauberk",4:"whhauberk1",5:"whhauberk2"}
pants_m = {1:"mtrouser",2:"mhtrouser",3:"mchoses",4:"mleggings",5:"mhleggings1",6:"mhleggings2"}
pants_w = {1:"wskirt",2:"wtrouser",3:"whtrouser",4:"wchoses",5:"wleggings",6:"whleggings1",7:"whleggings2"}
boots_m = {1:"mshoes",2:"mlboots"}
boots_w = {1:"wshoes",2:"wlboots"}
helm_m = {1:"mhelm1",2:"mhelm2",3:"mhelm3",4:"mhelm4",5:"nmhelm1",6:"nmhelm2",7:"nmhelm3",8:"nmhelm4",9:"mhhelm1",10:"mhhelm2",11:"mhcap1",12:"mhcap2"}
helm_w = {1:"whelm1",4:"whelm4",5:"nwhelm1",6:"nwhelm2",7:"nwhelm3",8:"nwhelm4",9:"whhelm1",10:"whhelm2",11:"whcap1",12:"whcap2"}
mantle_m = {1:"mmantle01",2:"mmantle02",3:"mmantle03",4:"mmantle04",5:"mmantle05",6:"mmantle06"}
mantle_w = {1:"wmantle01",2:"wmantle02",3:"wmantle03",4:"wmantle04",5:"wmantle05",6:"wmantle06"}

# Swords batch: msw/wsw.pak
msw_batch = {1:56*0, 2:56*1, 3:56*2, 4:56*3, 6:56*5, 7:56*6, 8:56*7, 9:56*8, 10:56*9, 11:56*10, 12:56*11}
sword_m = {appr: ("msw", off) for appr, off in msw_batch.items()}
sword_m[5] = ("mswx", 0)
sword_m[13] = ("msw2", 0)
sword_m[14] = ("msw3", 0)
sword_m[15] = ("mstormbringer", 0)
sword_m[16] = ("mdarkexec", 0)
sword_m[17] = ("mklonessblade", 0)
sword_m[18] = ("mklonessastock", 0)
sword_m[19] = ("mdebastator", 0)
sword_m[29] = ("mlightblade", 0)
sword_m[33] = ("mbshadowsword", 0)

wsw_batch = {1:56*0, 2:56*1, 3:56*2, 4:56*3, 6:56*5, 7:56*6, 8:56*7, 9:56*8, 10:56*9, 11:56*10, 12:56*11}
sword_w = {appr: ("wsw", off) for appr, off in wsw_batch.items()}
sword_w[5] = ("wswx", 0)
sword_w[13] = ("wsw2", 0)
sword_w[14] = ("wsw3", 0)
sword_w[15] = ("wstormbringer", 0)
sword_w[16] = ("wdarkexec", 0)
sword_w[17] = ("wklonessblade", 0)
sword_w[18] = ("wklonessastock", 0)
sword_w[19] = ("wdebastator", 0)
sword_w[29] = ("wlightblade", 0)
sword_w[33] = ("wbshadowsword", 0)

axe_m = {20:("maxe1",0),21:("maxe2",0),22:("maxe3",0),23:("maxe4",0),24:("maxe5",0),25:("mpickaxe1",0),26:("maxe6",0),27:("mhoe",0),28:("mklonessaxe",0),30:("mhammer",0),31:("mbhammer",0),32:("mbabhammer",0)}
axe_w = {20:("waxe1",0),21:("waxe2",0),22:("waxe3",0),23:("waxe4",0),24:("waxe5",0),25:("wpickaxe1",0),26:("waxe6",0),27:("whoe",0),28:("wklonessaxe",0),30:("whammer",0),31:("wbhammer",0),32:("wbabhammer",0)}
wand_m = {34:("mberserkwand",0),35:("mstaff1",0),36:("mstaff2",0),37:("mstaff3",0),38:("mremagicwand",0),39:("mklonesswand",0)}
wand_w = {34:("wberserkwand",0),35:("wstaff1",0),36:("wstaff2",0),37:("wstaff3",0),38:("wremagicwand",0),39:("wklonesswand",0)}
bow_m = {40:("mbo",56*0),41:("mbo",56*1),42:("mdirectbow",0),43:("mfirebow",0)}
bow_w = {40:("wbo",56*0),41:("wbo",56*1),42:("wdirectbow",0),43:("wfirebow",0)}
shield_m = {appr: ("msh", (appr-1)*7) for appr in range(1,10)}
shield_w = {appr: ("wsh", (appr-1)*7) for appr in range(1,10)}


def get_weapon_info(appr, gender):
    for table in ([sword_m, axe_m, wand_m, bow_m] if gender == "male" else [sword_w, axe_w, wand_w, bow_w]):
        if appr in table:
            return table[appr]
    return None


def get_equip_info(eqpos, appr, gender):
    """Returns (pak_file, first_sprite, type) where type is 'armor', 'weapon', or 'shield'"""
    if eqpos in (2, 13):  # Body / FullBody
        table = body_m if gender == "male" else body_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 3:
        table = arms_m if gender == "male" else arms_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 4:
        table = pants_m if gender == "male" else pants_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 5:
        table = boots_m if gender == "male" else boots_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 1:
        table = helm_m if gender == "male" else helm_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 12:
        table = mantle_m if gender == "male" else mantle_w
        if appr in table: return (table[appr], 0, "armor")
    elif eqpos == 7:  # Shield
        table = shield_m if gender == "male" else shield_w
        if appr in table:
            pak, off = table[appr]
            return (pak, off, "shield")
    elif eqpos in (8, 9):  # Weapon
        result = get_weapon_info(appr, gender)
        if result:
            pak, off = result
            return (pak, off, "weapon")
    return None


# ---- Query items ----
cur.execute("""
    SELECT item_id, name, item_type, equip_pos, sprite, sprite_frame, appr_value, item_color
    FROM items
    ORDER BY equip_pos, appr_value, item_color, item_id
""")
rows = cur.fetchall()


def strip_mw(name):
    return re.sub(r"\s*\([MW]\)\s*$", "", name).strip()


# Group equippable by (equip_pos, appr_value), non-equippable by (sprite, sprite_frame)
equip_groups = defaultdict(list)
icon_groups = defaultdict(list)
for r in rows:
    item_id, name, itype, eqpos, spr, frame, appr, color = r
    if eqpos == 0:
        icon_groups[(spr, frame)].append((item_id, name, color))
    else:
        equip_groups[(eqpos, appr)].append((item_id, name, spr, frame, color))

entries = []

for key in sorted(equip_groups.keys()):
    eqpos, appr = key
    items = equip_groups[key]
    rep_name = strip_mw(items[0][1])

    male_info = get_equip_info(eqpos, appr, "male")
    female_info = get_equip_info(eqpos, appr, "female")

    pak = {}
    equip_type = None
    first_sprite_m = 0
    first_sprite_f = 0

    if male_info:
        pak_file, first_sprite_m, equip_type = male_info
        pak["male"] = pak_file + ".pak"
    if female_info:
        pak_file, first_sprite_f, equip_type_f = female_info
        pak["female"] = pak_file + ".pak"
        if not equip_type:
            equip_type = equip_type_f

    # Build animations based on equipment type
    if equip_type == "armor":
        animations = make_armor_animations()
    elif equip_type == "weapon":
        animations = make_weapon_animations(first_sprite_m)
    elif equip_type == "shield":
        animations = make_shield_animations(first_sprite_m)
    else:
        animations = None

    all_names = list(dict.fromkeys(strip_mw(n) for _, n, _, _, _ in items))

    entry = {
        "name": rep_name,
        "equip_slot": equip_names.get(eqpos, "?"),
        "equip_pos": eqpos,
        "appearance_value": appr,
        "pak": pak,
        "animations": animations,
        "all_items": all_names,
    }
    entries.append(entry)

for key in sorted(icon_groups.keys()):
    spr, frame = key
    items = icon_groups[key]
    all_names = list(dict.fromkeys(n for _, n, _ in items))

    entry = {
        "name": items[0][1],
        "equip_slot": "None",
        "equip_pos": 0,
        "icon": {"sprite": spr, "frame_rect": frame},
        "animations": None,
        "all_items": all_names,
    }
    entries.append(entry)

entries.sort(key=lambda e: (e["equip_pos"], e.get("appearance_value", -1)))

output = {
    "description": "Unique item visuals with 12 animation types. Armor: pak_sprite=bodyPose index, stride=frames per direction. Weapons: pak_sprite=first_sprite + weaponPose*8, 8 sprites per pose (one per dir). Shields: pak_sprite=first_sprite + shieldPose. null=not rendered in that animation.",
    "total_unique_visuals": len(entries),
    "equippable": len([e for e in entries if e["equip_pos"] > 0]),
    "non_equippable": len([e for e in entries if e["equip_pos"] == 0]),
    "visuals": entries,
}

with open("Z:/Helbreath-3.82/PLANS/Item_Animation_Map.json", "w") as f:
    json.dump(output, f, indent=2)

print("Written %d unique visuals to PLANS/Item_Animation_Map.json" % len(entries))
print("  Equippable: %d" % output["equippable"])
print("  Non-equippable: %d" % output["non_equippable"])
db.close()

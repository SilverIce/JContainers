; returns the form for the item worn at the specified slotMask
; use Armor.GetMaskForSlot() to generate appropriate slotMask
Form Function GetWornForm(int slotMask) native

; returns the itemId for the item worn at the specified slotMask
int Function GetWornItemId(int slotMask) native

; returns the object currently equipped in the specified location
; 0 - left hand
; 1 - right hand
; 2 - shout
Form Function GetEquippedObject(int location) native

; returns the itemId of the object currently equipped in the specified hand
; 0 - left hand
; 1 - right hand
int Function GetEquippedItemId(int location) native

; returns the number of added spells for the actor
Int Function GetSpellCount() native

; returns the specified added spell for the actor
Spell Function GetNthSpell(int n) native

; Updates an Actors meshes (Used for Armor mesh/texture changes and face changes)
; DO NOT USE WHILE MOUNTED
Function QueueNiNodeUpdate() native

; Updates an Actors head mesh
Function RegenerateHead() native

int Property EquipSlot_Default = 0 AutoReadOnly
int Property EquipSlot_RightHand = 1 AutoReadOnly
int Property EquipSlot_LeftHand = 2 AutoReadOnly

; equips item at the given slot
Function EquipItemEx(Form item, int equipSlot = 0, bool preventUnequip = false, bool equipSound = true) native

; equips item with matching itemId at the given slot
Function EquipItemById(Form item, int itemId, int equipSlot = 0, bool preventUnequip = false, bool equipSound = true) native

; unequips item at the given slot
Function UnequipItemEx(Form item, int equipSlot = 0, bool preventEquip = false) native

; Adds a headpart, if the type exists it will replace, must not be misc type
; Beware: This function also affects the ActorBase
Function ChangeHeadPart(HeadPart hPart) native

; Visually updates the actors weight
; neckDelta = (oldWeight / 100) - (newWeight / 100)
; Neck changes are player persistent, but actor per-session
; Weight itself is persistent either way so keep track of your 
; original weight if you use this for Actors other than the player
; DO NOT USE WHILE MOUNTED
Function UpdateWeight(float neckDelta) native

; Returns whether the actors AI is enabled
bool Function IsAIEnabled() native

; Returns whether the actor is currently swimming
bool Function IsSwimming() native

; Sheathes the actors weapon
Function SheatheWeapon() native

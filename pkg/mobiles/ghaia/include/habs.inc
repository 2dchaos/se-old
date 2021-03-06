include "include/utils";
include "include/facings";
include "include/shapes";

var cooldown_habs := struct{};

function SetHabCd(hab_name, cd)
   cooldown_habs[hab_name] := polcore().systime + (cd);
endfunction

function GetHabCd(hab_name)
   if (cint(polcore().systime) >= cint(cooldown_habs[hab_name]))
      return 0;
   endif

   return 1;
endfunction

function UseHabs(habs := array{})
   habs += GetConfigStringArray(npccfgfile, "EspecialAttack");
   if (!habs || !habs[1])
      return;
   endif

   foreach hab in habs
      sleepms(2);
      hab := SplitWords(hab);
      var hab_name  := hab[1];

      if (GetHabCd(hab_name))
         sleepms(2);
         continue;
      endif

      // if ("Healer" in custombehaviors && hab_name != "curar" && RandomInt(10) < 2)
      //    continue;
      // endif

      var hab_cooldown := cint(hab[2]);
      var hab_chance  := cint(hab[3]);
      var hab_dice     := hab[4];
      var hab_target   := hab[5];
      var hab_special  := hab[6];

      var chance_try := Randomint(100)+1;
      if (hab_chance != 100 && chance_try > hab_chance )
         SetHabCd(hab_name, 5);
         continue;
      endif

      if (hab_dice["no_roll"])
      elseif (hab_dice["d"])
         hab_dice := RandomDiceRoll(hab_dice);
      else
         hab_dice := cint(hab_dice);
      endif

      var res := PlaySpecialHab(hab_name, hab_cooldown, hab_dice, hab_target, hab_special);
      if (res) break; endif
   endforeach
endfunction

function PlaySpecialHab(hab_name, hab_cooldown := 0, hab_dice := 0, hab_target := "Enemy", hab_special := 0)
   if (hab_target == "Enemy")
      hab_target := me.opponent;
   elseif (hab_target == "Enemies")
      hab_target := GetEnemiesNear(me, 10);
      if (hab_target.size() < 1) return 0; endif
   elseif (hab_target == "RandomEnemy")
      var enemies_near := GetEnemiesNear(me, 10);
      if (enemies_near.size() < 1) return 0; endif
      hab_target := enemies_near.randomentry();
   elseif (hab_target == "Ally")
      foreach ally in GetAlliesNear(me, 6)
         if (hab_name == "curar")
            if (AP_GetVital(ally, HITS) == AP_GetVitalMaximumValue(ally, HITS))
               continue;
            endif
         endif
         hab_target := ally;
         break;
      endforeach
   elseif (hab_target == "AllyUnderAttack")
      hab_target := 0;
      foreach ally in GetAlliesNear(me, 6)
         if (ally.opponent.opponent == ally)
            hab_target := ally;
            break;
         endif
      endforeach
      if (!hab_target) return 0; endif
   elseif (hab_target == "Self")
      hab_target := me;
   endif

   var res;
   case (hab_name)
      "summon":         res := SummonCreature(cint(hab_dice), hab_special); break;
      "agarrar":        res := NPC_Agarrar(hab_dice, hab_target, cint(hab_special)); break;
      "cancaoSedutora": res := CancaoSedutora(hab_target, hab_dice); break;
      "furtividade":    res := NPC_Sneak(); break;
      "metamorfose":    res := Metamorfose(hab_special); break;
      "curar":          res := Healing(hab_target, hab_dice); break;
      "curar_area":     res := AreaHeal(hab_dice); break;
      "roubar":         res := Roubar(hab_target); break;
      "gumeArea":        res := gumeArea(hab_target, hab_dice, cint(hab_special)); break;
      "rajada_acida":   res := RajadaAcida(hab_dice, hab_target, cint(hab_special)); break;
      "electricball": res := ElectricBall(hab_target, hab_special); break;
      "teia":           res := SpitWeb(hab_special, hab_target, cint(hab_special)); break;
      "teleport":       res := teleport(hab_target, hab_special); break;
      "constrictSpell": res := constrict(hab_target, hab_dice, cint(hab_special)); break;
      "charming":       res := charm(hab_target, hab_dice, cint(hab_special)); break;
      "SoproEuforia":   res := euforic(hab_target, hab_dice, cint(hab_special)); break;
      "hunterMark":     res := hunterMark(hab_target); break;
      "throwBomb":      res := throwBomb(hab_target, hab_dice, hab_special); break;
      "evasao":         res := evasion(hab_target); break;
      "fireWall":       res := fireWall(hab_target, hab_dice, cint(hab_special)); break;
      "Iceberg":      res := Iceberg(hab_target, hab_dice, cint(hab_special)); break;
      "scald":        res := scald(hab_target, hab_dice, cint(hab_special)); break;
      "nuvem":        res := nuvem(hab_dice); break;
      "pullCloser":   res := pullCloser(hab_dice, cint(hab_special)); break;
      "electricCone": res := electricCone(hab_target, hab_dice, cint(hab_special)); break;
      "fireCone": res := fireCone(hab_target, hab_dice, cint(hab_special)); break;
      "cavar": res := cavar(hab_target, hab_dice, cint(hab_special)); break;
      "saltar": res := jumpAttack(hab_target, hab_dice); break;
      "mina": res := createMine(hab_dice, cint(hab_target), cint(hab_special));
      "dash": res := dashAttack(hab_target, hab_dice, cint(hab_special));
      "comerCriatura": res := eatCreature(hab_dice, hab_special);
      
      // "pelede_de_arvore": res := PeleDeArvore(hab_dice); break;
      default:
         Print("{} npc hab not exist".format(hab_name));
         SetHabCd(hab_name, 150);
         break;
   endcase

   if (hab_cooldown && res)
      SetHabCd(hab_name, CInt(hab_cooldown));
      return 1;
   else
      SetHabCd(hab_name, 5);
   endif
endfunction

function eatCreature(hab_radius, cria_template)
   var cria := 0;
   foreach mob in ListMobilesNearLocation(me.x, me.y, me.z, hab_radius)
      if (mob.npctemplate[cria_template])
         cria := mob;
         break;
      endif
      sleepms(2);
   endforeach

   if (!cria)
      return 0;
   endif

   PrintText(me, "*come {}".format(cria.name));
   SetObjProperty(cria, "GuardKill", 1);
   SetObjProperty(cria, "nocorpse", 1);
   var cria_hp := AP_GetVital(cria, HITS);
   ApplyRawDamage(cria, 500);

   HealFLS(me, cria_hp);
   return 1;
endfunction

function dashAttack(targ, dmg, dist)
   printtext(me, "*pega impulso*");
   sleepms(750);
   var last_loc := array{me.x, me.y};
   var targ_face := GetFacing(me.x, me.y, targ.x, targ.y);
   var mod := GetModByFacing(targ_face);
   me.facing := targ_face;
   var mobs := array{};
   for i := 0 to dist
      if (me.paralyzed || me.frozen) break; endif
      MoveObjectToLocation(me, me.x+mod[1], me.y+mod[2], me.z, me.realm);
      // RunTowardLocation(me.x+mod[1], me.y+mod[2]);
      if (me.x == last_loc[1] && me.y == last_loc[2]) break; endif
      mobs += GetEnemiesNear(me, 0);
      last_loc := array{me.x, me.y};
      if (i != dist)
         sleepms(110);
      endif
   endfor

   foreach mob in mobs
      DamageFLS(mob, dmg, DMG_BLUDGE, me);
      if (rollResistDice(mob, CONSTITUTION) <= 14)
         TS_StartTimer(mob, "tombar", 2);
      endif
      sleepms(2);
   endforeach

   return 1;
endfunction

function jumpAttack(targ, dmg)
   if (Distance(me, targ) < 3 || Distance(me, targ) > 6)
      return 0;
   endif
   printtext(me, "*salta*");
   MoveObjectToLocation(me, me.x, me.y, me.z+8, me.realm, MOVEOBJECT_FORCELOCATION);
   sleepms(600);
   MoveObjectToLocation(me, targ.x, targ.y, targ.z, me.realm, MOVEOBJECT_FORCELOCATION);
   DamageFLS(targ, dmg, DMG_BLUDGE, me);
   return 1;
endfunction

function nuvem(timing)
   var enemies := GetEnemiesNear(me, 10);
   var av_enemies := array{};
   foreach en in enemies
      if (Distance(me, en) > 3)
         av_enemies.append(en);
      endif
      sleepms(2);
   endforeach

   if (av_enemies.size() < 1)
      return;
   endif

   var targ := av_enemies.randomentry();
   var item := CreateItemAtLocation(targ.x, targ.y, targ.z, "nuvem", 1);
   item.color := 904;
   SetObjProperty(item, "expiretime", polcore().systime + timing);
   SetObjProperty(item, "owner", me.serial);
   return 1;
endfunction

function pullCloser(dist, times)
   PrintText(me, "Batendo Asas");
   for i := 1 to (cint(times) *2)
      PerformAction(me, cint(0x06));
      if (i % 2 == 0)
         var enemies := GetEnemiesNear(me, dist);
         foreach mob in enemies
            sleepms(2);
            if ( Distance(me, mob) < 1 )
               continue;
            endif
            if (CheckLineOfSight(me, mob))
               SendSysMessageEx(mob, "Voc� � puxado pelo vento");
               var point := GetModByFacing(GetFacing(me.x, me.y, mob.x, mob.y));
               point[1] *= -1;
               point[2] *= -1;
               MoveObjectToLocation(mob, mob.x+point[1], mob.y+point[2], mob.z, mob.realm, MOVEOBJECT_FORCELOCATION);
               MoveObjectToLocation(mob, mob.x+point[1], mob.y+point[2], mob.z, mob.realm, MOVEOBJECT_FORCELOCATION);
            endif
         endforeach
      endif
      sleepms(200);
   endfor
   PrintText(me, "Batendo Asas");
   return 1;
endfunction


function electricCone(mobile, dmg, dist)
   if (Distance(me, mobile) > 2)
      return 0;
   endif
   
   PrintText(me, "*baforada*");
   var points := GetConeCoordsInFront(me.x, me.y, me.facing, dist);
   points.erase(points.size());
   points.erase(points.size()-2);
   dmg := "{}d4".format(dmg);

   foreach p in points
      var mobs := ListMobilesNearLocation(p[1], p[2], me.z+2, 0, me.realm);
      foreach m in mobs
         if (CanFight(me, m))
            DamageFLS(m, RandomDiceRoll(dmg), DMG_ENERGY, me);
         endif
         sleepms(2);
      endforeach
      LightningEffect(struct{ "x" := p[1], "y" := p[2], "z" := me.z, "realm" := me.realm}, 50);
      sleepms(2);
   endforeach

   return 1;
endfunction

function fireCone(mobile, dmg, dist)
   if (Distance(me, mobile) > 2)
      return 0;
   endif
   
   PrintText(me, "*baforada*");
   var points := GetConeCoordsInFront(me.x, me.y, me.facing, dist);
   dmg := "{}d4".format(dmg);
   foreach p in points
      var item := CreateItemAtLocation(p[1], p[2], me.z+1, 0x3996);
      SetObjProperty(item, "DamageDice", dmg);
      item.SetDuration(0);
      item.SetOwner(me);
      sleepms(2);
   endforeach

   return 1;
endfunction

function throwBomb(mobile, hab_dice, bombs)
   if (Distance(mobile, me) > 12 || !CheckLineOfSight(me, mobile)) return; endif

   bombs := SplitWords(bombs, "|");
   var item := CreateItemAtLocation(me.x, me.y, me.z, bombs.randomentry());
   start_script(":alchemy:potions/explosionTimer", array{item, me});
   SetObjProperty(item, "#bomb", 1);
   PrintText(me, "*joga um frasco*");

   PlayMovingEffectXYZ( me.x, me.y, me.z+15, mobile.x, mobile.y, mobile.z, 0x0f09, 8, 0, 0 );
   MoveObjectToLocation(item, mobile.x, mobile.y, mobile.z, mobile.realm, MOVEOBJECT_FORCELOCATION );
   EraseObjProperty(item, "#bomb");
   return 1;
endfunction

function evasion(mobile)
   if (Distance(mobile, me) > 2)
      return 0;
   endif

   var point := RandomPlace(mobile, 1);
   MoveObjectToLocation(me, point.x, point.y, mobile.z, mobile.realm, MOVEOBJECT_FORCELOCATION);
   me.facing := GetFacing(me.x, me.y, mobile.x, mobile.y);
   return 1;
endfunction


function hunterMark(mobile)
   PlayStationaryEffectEx(mobile.x, mobile.y, mobile.z+20, mobile.realm, 0x9E29, 0, 15, 1650);
   foreach npc in GetAlliesNear(mobile, 5)
      if (npc.alignment == 1)
         if (NPC_GetNPCConfig(npc).Category == "beast")
            npc.setAggro(mobile.serial, 40);
         endif
      endif
      sleepms(2);
   endforeach
   
   return 1;
endfunction

function fireWall(mobile, dmg_dice, diff := 14)
   if (Distance(mobile, me) > 12)
      return 0;
   endif
   
   if (!playCastAnimation())
      return 1;
   endif

   var direction := GetFacing(me.x, me.y, mobile.x, mobile.y);
   var field := cint(direction / 2);
   if (field > 4)
      field /= 2;
   endif

   var flame := "FireFieldNS";
   if (field % 2)
      flame := "FireFieldEW";
   endif

   var pos := GetCoordsInLine(me.x, me.y, mobile.x, mobile.y);
   pos.erase(1);

   var z := me.z;
   foreach p in pos
      if (mobile.z > z)
         z += 1;
      elseif (mobile.z < z)
         z -= 1;
      endif
      var item := CreateItemAtLocation(p.x, p.y, z, flame);
      item.SetDuration(0);

      var mobs := GetEnemiesNear(item, 0);
      foreach mob in mobs
         if (rollResistDice(mob, DEXTERITY) > diff)
            PrintText(mob, "*resistido*", SSM_INFO);
         else
            DamageFLS(mob, dmg_dice, DMG_FIRE, me);
         endif
      endforeach
      sleepms(150);
   endforeach

   return 1;
endfunction

function gumeArea(targs, dmg, chance)
   if (!playCastAnimation())
      return 1;
   endif

   PrintText(me, "*o gelo come�a a se formar ao redor*");
   PlaySoundEffect(me, 0x24B);

   foreach targ in targs
      var mod := GetModByFacing(GetFacing(me.x, me.y, targ.x, targ.y));
      PlayStationaryEffectEx(me.x+mod[1], me.y+mod[2], me.z, me.realm, GFX_GLOW, 9, 20, 0);
      sleepms(2);
   endforeach

   sleepms(900);

   foreach targ in targs
      PlayMovingEffectXYZEx(me.x, me.y, me.z+10, targ.x, targ.y, targ.z+10, me.realm, 0x1ECD, 5, 25, 2054);
      if (rollResistDice(targ, DEXTERITY) > chance)
         SendSystemReport(targ, "*esquiva*", SSM_INFO);
         SendSystemReport(targ, "*esquiva*", SSM_FAIL, me);
      else
         DamageFLS(targ, dmg, DMG_COLD, me);
      endif
      sleepms(2);
   endforeach
endfunction

function createMine(dmg, amount, is_fire := 0)
   if (!playCastAnimation())
      return 1;
   endif

   for i := 1 to amount
      var item;
      while (!item)
         var point := randomPlace(me, 3);
         item := CreateItemAtLocation(point.x, point.y, me.z, 0x307f);
         SetObjProperty(item, "DamageDice", "{}d4".format(dmg));
         item.SetOwner(me);
         item.SetDuration(60);
         if (is_fire)
            item.SetEffect(DMG_FIRE);
         endif
         sleepms(2);
      endwhile
      sleepms(2);
   endfor

   return 1;
endfunction

function Iceberg(mobile, ice_damage, diff)
   var stalagmites := array{0x08E0, 0x08E1, 0x08E4, 0x08E5, 0x08E0};
   var st_item := CreateItemAtLocation(mobile.x, mobile.y, mobile.z-1, 0x08E0);
   st_item.graphic := stalagmites.randomentry();
   st_item.color := 2051;
   st_item.SetDuration(2);

   PlaySoundEffect(mobile, 0x20E);
   PlayStationaryEffectEx( st_item.x, st_item.y, st_item.z, st_item.realm, 0x3779, 1, 16);
   var enemies := GetEnemiesNear(mobile, 1);
   foreach mob in enemies
      if (rollResistDice(mob, DEXTERITY) > diff)
         PrintText(mob, "*esquiva*", SSM_INFO);
      else
         DamageFLS(mob, ice_damage, DMG_COLD, me);
      endif
   endforeach
   return 1;
endfunction

function charm(mobile, ts_time, diff)
   if (!playCastAnimation())
      return 1;
   endif

   if (mobile.isA(POLCLASS_MOBILE))
      mobile := array{mobile};
   endif

   foreach mob in mobile
      sleepms(2);
      PlayObjectCenteredEffect(me, 0x37B9, 2, 15);

      if (rollResistDice(mob, INTELLIGENCE) > diff)
         SendSystemReport(mob, "*resistido*", SSM_INFO);
         continue;
      endif

      TS_StartTimer(mob, "charmed", cint(ts_time), diff, me);
   endforeach

   return 1;
endfunction

function constrict(mobile, ts_time, diff)
    
   if (!playCastAnimation())
      return 1;
   endif

   PlayObjectCenteredEffect(me, 0x37B9, 2, 15);
   PrintText(mobile, "*raizes tentam se prender a voc�*");
   if (rollResistDice(mobile, STRENGTH) > diff)
      PrintText(mobile, "*resistido*", SSM_INFO);
      return 1;
   endif

   TS_StartTimer(mobile, "constricted", ts_time, diff);
   return 1;
endfunction

function euforic(mobile, ts_time, diff)
   if (!playCastAnimation())
      return 1;
   endif

   if (Distance(me, mobile) > 2)
      return 0;
   endif

   PrintText(me, "*baforada m�gica*");
   var points := GetConeCoordsInFront(me.x, me.y, me.facing, 3);
   points.erase(points.size());
   points.erase(points.size()-2);
   foreach p in points
      var mobs := ListMobilesNearLocation(p[1], p[2], me.z+2, 0, me.realm);
      foreach m in mobs
         if (CanFight(me, m))
            if (rollResistDice(m, CONSTITUTION) > diff)
               PrintText(m, "*resistido*", SSM_INFO);
            else
               TS_StartTimer(m, "euforic", ts_time, diff);
            endif
         endif
         sleepms(2);
      endforeach
      PlayStationaryEffectEx( p[1], p[2], me.z+2, me.realm, 0x9DAC, 7, 10, 1953);
      sleepms(2);
   endforeach

   return 1;
endfunction

function scald(mobile, dmg, diff)
   if (!playCastAnimation())
      return 1;
   endif

   if (Distance(me, mobile) > 2)
      return 0;
   endif

   PrintText(me, "*solta um jato de ar quente*");
   var points := GetConeCoordsInFront(me.x, me.y, me.facing, 2);
   foreach p in points
      var mobs := ListMobilesNearLocation(p[1], p[2], me.z+2, 0, me.realm);
      foreach m in mobs
         if (CanFight(me, m))
            if (rollResistDice(m, CONSTITUTION) > diff)
               PrintText(m, "*resistido*", SSM_INFO);
            else
               DamageFLS(m, dmg, DMG_FIRE, me);
               AP_ConsumeVital(m, STAMINA, 6);
            endif
         endif
         sleepms(2);
      endforeach
      PlayStationaryEffectEx( p[1], p[2], me.z+2, me.realm, 0x372A, 7, 18, 2150);
      sleepms(2);
   endforeach

   return 1;
endfunction

function summonCreature(qty, creatures)
   if (!playCastAnimation())
      return 1;
   endif

   creatures := SplitWords(creatures);
   for i := 1 to qty
      var rad := randomPlace(me, 5);
      var summoned := CreateNpcFromTemplate(":ghaia:{}".format(creatures.randomentry()), rad.x, rad.y, me.z, 0, me.realm);
      ghaiaattack(summoned, me.opponent);
      SetObjProperty(summoned, "nocorpse", 1);
      TS_StartTimer(summoned, "summon", 60);
   endfor

   return 1;
endfunction

function CancaoSedutora(mobiles, dice)
   if (!playCastAnimation())
      return 1;
   endif

   if (!mobiles[1]) return 0; endif
   sleep(1);
   PerformAction(me, 0x06);
   PlaySoundEffect(me, 0x58C);
   PrintText(me, "*cantando*");
   foreach mob in mobiles
      if (rollResistDice(mob, WISDOM) <= dice)
         TS_StartTimer(mob, "paralysis", 5);
      else
         SendSystemReport(mob, "*resistido*", SSM_INFO);
      endif
      sleepms(2);
   endforeach
   sleep(2);
   PerformAction(me, 0x06);
   sleep(2);
   PerformAction(me, 0x06);
   PrintText(me, "*cantando*");
   return 1;
endfunction

function NPC_Agarrar(dice, targ, difficulty)
   if (Distance(me, targ) > 2)
      return 0;
   endif

   PrintTexT(me, "*tenta agarrar {}*".format(targ.name));
   if (rollResistDice(targ, DEXTERITY) <= difficulty)
      PrintText(targ, "*agarrado*", SSM_FAIL);

      TS_StartTimer(targ, "paralysis", dice);
      TS_StartTimer(me, "paralysis", dice);
      MoveObjectToLocation(me, targ.x, targ.y, targ.z, targ.realm, MOVEOBJECT_FORCELOCATION);
      for i := 1 to dice
         if (!me)
            return;
         endif
         sleep(1);
         DamageFLS(targ, RandomDiceRoll("1d4"), DMG_BLUDGE, me);
      endfor
   else
      PrintText(targ, "*resistido*".format(me.name), SSM_INFO);
   endif
   
   return 1;
endfunction

function Metamorfose(hab_special)
   if ("MetamorfoseClose" in custombehaviors)
      if (Distance(me, me.opponent) > 4)
         return 0;
      endif
   endif

   if (!playCastAnimation())
      return 1;
   endif

   var monsters := SplitWords(hab_special);
   var choose_monster := monsters.randomentry();
   PrintText(me, "*metamorfose*");
   me.frozen := 1;
   PlaySoundEffect(me, 0x656);
   PlayStationaryEffect(me.x, me.y, me.z, GFX_SMOKE, 10, 10, 0);
   SetObjProperty(me, "metamorfose", choose_monster);
   SetObjProperty(me, "oldgraphic", me.graphic);
   
   sleep(1);
   var meta_config := NPC_GetNPCConfig(":ghaia:{}".format(choose_monster));
   me.graphic := meta_config.objtype;
   me.frozen := 0;
   TS_StartTimer(me, "metamorfose", 30);
   return 1;
endfunction

function Healing(targ, heal)
   if (!targ.isA(POLCLASS_MOBILE))
      return 0;
   endif
   if (!playCastAnimation())
      return 1;
   endif

   PlaysoundEffect(me, 1137);
   PlayObjectCenteredEffect( targ, 0x373a, 7, 0x10 );
   HealDamage(targ, heal);
   return 1;
endfunction

function AreaHeal(heal)
   if (!playCastAnimation())
      return 1;
   endif

   PlaysoundEffect(me, 1137);
   foreach ally in GetAlliesNear(me, 6)
      PlayObjectCenteredEffect( ally, 0x373a, 7, 0x10 );
      HealDamage(ally, heal);
   endforeach

   return 1;
endfunction

function NPC_Sneak()
   PrintText(me, "*esconde*");
   GrantPrivilege(me, "runwhilestealth");
   me.enable("runwhilestealth");
   me.hidden := 1;
   me.stealthsteps := 15;
   return 1;
endfunction

function Roubar(targ)
   if (!IsBehind(targ, me.x, me.y) || Distance(me, targ) > 1)
      return 0;
   endif

   var roll := rollAttrDice(me, DEXTERITY) + GetSkillBonus(me, FURTO);
   foreach item in EnumerateItemsInContainer(targ.backpack, ENUMERATE_ROOT_ONLY)
      var diff := 10 + CInt(item.amount * 0.3) + cint(item.weight/2);
      if (roll > diff)
         MoveItemToContainer(item, me.backpack);
         var user_percep := rollAttrDice(targ, WISDOM) + GetSkillBonus(targ, PERCEPCAO);
         if (user_percep > roll)
            SendSystemReport(me, "*pegou item da sua mochila*", SSM_FAIL, targ);
         endif
         break;
      endif
      sleepms(2);
   endforeach
   
   return 1;
endfunction

function RajadaAcida(dmg, targ, difficulty)
   if (!targ || Distance(me, targ) < 3  || !CheckLineOfSight(me, targ))
      return 0;
   endif

   PrintText(me, "*cospe �cido*");
   var phase_coords := GetCoordsInLine(me.x, me.y, targ.x, targ.y);
   foreach coord in phase_coords
      var acid := CreateItemAtLocation(coord.x, coord.y, me.z, 0xA370);
      SetObjProperty(acid, "AcidDice", dmg);
      SetObjProperty(acid, "AcidDiff", difficulty);
      sleepms(50);
   endforeach

   if (rollResistDice(targ, DEXTERITY) <= difficulty)
      DamageFLS(targ, dmg, DMG_POISON, me);
   else
      DamageFLS(targ, dmg/2, DMG_POISON, me);
   endif

   return 1;
endfunction

function playCastAnimation(forced := 0)
   if ("CastAnimation" in custombehaviors || forced)
      for i := 1 to (RandomDiceRoll("1d4"))
         if (TS_GetTimerInfo(me, "tombar") || TS_GetTimerInfo(me, "paralyze"))
            return 0;
         endif
         PerformAction(me, ANIM_CAST_DIR);
         sleepms(650);
      endfor
   endif

   return 1;
endfunction

function ElectricBall(targ, dmg_dice)
   if (!targ)
      return 0;
   endif
   PrintText(me, "*prepara uma bola de energia*");
   playCastAnimation();
   sleep(2);
   var newball := CreateItemAtLocation(me.x, me.y, me.z+2, 0xFFBE);
   newball.SetOwner(me);
   newball.SetDirection(targ);
   newball.SetDuration(8);
   newball.SetType(1);
   newball.SetEffect(0x372A);
   newball.SetBallSound(0x666, 0);
   newball.SetBallGraphic(13935, 1990);
   newball.SetDmgInfo(struct{ "dice":= dmg_dice, "type" := DMG_ENERGY });
endfunction


// function throwItem(targ)
//    if ( dist < 3 || dist > 15 )
//       return 0;
//    elseif ( !CheckLineOfSight(me, targnpc, opponent) )
//       return 0;
//    elseif ( CInt( GetCooldown(npc, "throws" )) != 0 )
//       return 0;
//    endif

//    npc.frozen := 1;
//    sleepms(50);

//    if (throwinfo.anim)
//       PerformAction(npc, throwinfo.anim);
//    else
//       PerformAction(npc, 4);
//    endif

//    sleep(1);

//    PlayMovingEffect(npc, opponent, throwinfo.item, 15);
//    npc.frozen := 0;

//    var item := CreateItemAtLocation(opponent.x, opponent.y, opponent.z, throwinfo.item, 1);
//    item.movable := 1;


//    var chance := GetAttribute(npc, "Dexterity") - GetAttribute(opponent, "Dexterity") / 3;

//    if (RandomInt(100) > chance) // Errou!

//       if (throwinfo.misssound)
//          PlaySoundEffect(npc, throwinfo.misssound);
//       else
//          PlaySoundEffect(npc, 0x137);
//       endif
//    else
//       DamageFLS(opponent, RandomDiceRoll(throwinfo.damage), DMG_PHYSICAL, npc);
//       PlaySoundEffect(npc, throwinfo.hitdamage);
//    endif

//    SetCooldown(npc, "throws", throwinfo.delay);


//    return 1;
// endfunction
function Firebreath(npc, opponent, byref dist)
   if ( dist < 5 || dist > 15 )
      return 0;
   elseif ( CInt( GetCooldown(npc, "breath" )) != 0 )
      return 0;
   elseif ( !CheckLineOfSight(me, targnpc, opponent) )
      return 0;
   endif

   TurnTowardLocation( opponent.x, opponent.y );
   //AI_Turn(npc, opponent, NETURN_TOWARD);
   npc.frozen := 1;
   PerformAction(npc, 12);
   PlaySoundEffect(npc, 0x16B);

   sleep(2);

   PlayMovingEffect(npc, opponent, GFX_LARGE_FIREBALL, 10, 10, 1);
   var coord_list := GetCoordsInLine(npc.x, npc.y, opponent.x, opponent.y);

   foreach coord in coord_list
      if ( _coord_iter < 4 ) // Dont make one where its standing
         continue;
      endif

      var field := CreateItemAtLocation(coord.x, coord.y, npc.z, "FireField_NS", 1);
      field.SetDuration(0);
      sleepms(105);
   endforeach

   foreach coord in GetCoordsInSquare(opponent.x, opponent.y, CInt(npccfgfile.BreathRadius))
      if ( RandomInt(2) != 1 ) // Don't place a fire field
         continue;
      endif

      var effects := array{GFX_EXPLODE_1, GFX_EXPLODE_2, GFX_EXPLODE_3};
      PlayStationaryEffect(coord.x, coord.y, npc.z, effects.randomentry(), 10, 10, 0);

      var fields := array{"FireFieldNS", "FireFieldEW"};
      var field := CreateItemAtLocation(coord.x, coord.y, opponent.z, fields.randomentry, 1);
      field.SetDuration(20);
      sleepms(5);
   endforeach
   
   PlaySoundEffect(opponent, 0x208);
   var chars := ListMobilesNearLocation(opponent.x, opponent.y, opponent.z, CInt(npccfgfile.BreathRadius), npc.realm);

   foreach mobile in chars
      DamageFLS(mobile, RandomDiceRoll(CInt(npccfgfile.BreathDamage)), DMG_FIRE, npc);
   endforeach
   npc.frozen := 0;

   SetCooldown(npc, "breath", CInt(npccfgfile.BreathDelay));
   return 1;
endfunction

function SpitWeb(difficulty, targ, range)
   range := cint(range);
   if (!targ || Distance(me, targ) > range )
      return 0;
   elseif ( !CheckLineOfSight(me, targ) )
      return 0;
   elseif ( targ.frozen )
      return 0;
   elseif (GetEquipmentByLayer(targ, 0x19))
      return 0;
   endif

   me.frozen := 1;
   sleepms(50);

   var web := array{3811, 3812, 3813, 3814}.randomentry();

   PlayMovingEffect(me, targ, web, 10);

   me.frozen := 0;
   TS_StartTimer(targ, "teia", 5, difficulty);
   return 1;
endfunction

function cavar(targ, diff, effect := 0x3728)
   if(!effect) effect := 0x3728; endif

   if (Distance(me, targ) < 3)
      return 0;
   endif

   PrintText(me, "*entra no subsolo*");
   PlayStationaryEffect(me.x, me.y, me.z, effect, 5, 5, 0);
   me.concealed := 1;
   TurnObjectToward(me, targ.x, targ.y);
   PlaySoundEffect(me, 0x1FF);
   sleep(1);
   MoveObjectToLocation(me, targ.x, targ.y, targ.z, me.realm, MOVEOBJECT_FORCELOCATION);
   me.concealed := 0;
   PrintText(me, "*surge embaixo de {}*".format(targ.name));

   var enemies := GetEnemiesNear(targ, 0);
   foreach enemie in enemies
      if (rollResistDice(enemie, DEXTERITY) <= diff)
         TS_StartTimer(enemie, "tombar", 10);
      endif
      sleepms(2);
   endforeach
      // PlayStationaryEffectHuefx( npc.x, npc.y, npc.z, CInt(npccfgfile.teleportEffect), 5, 5,  0, CInt(npccfgfile.teleportColor), CInt(npccfgfile.teleportAlpha));

   return 1;
endfunction

function teleport(targ, effect := 0x3728)
   if(!effect) effect := 0x3728; endif
   var phase_coords := GetCoordsInLine(me.x, me.y, targ.x, targ.y);
   phase_coords.Erase(phase_coords.Size()); 
   TurnObjectToward(me, targ.x, targ.y);
   PlaySoundEffect(me, 0x1FF);

   foreach coord in phase_coords
      PlayStationaryEffect(me.x, me.y, me.z, effect, 5, 5, 0);
      // PlayStationaryEffectHuefx( npc.x, npc.y, npc.z, CInt(npccfgfile.teleportEffect), 5, 5,  0, CInt(npccfgfile.teleportColor), CInt(npccfgfile.teleportAlpha));
      MoveObjectToLocation(me, coord.x, coord.y, targ.z, me.realm, MOVEOBJECT_FORCELOCATION);
      sleepms(20);
   endforeach

   return 1;
endfunction

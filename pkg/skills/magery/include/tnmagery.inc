include "include/say";
include "include/arrays";
include ":timedscripts:timedScripts";

function WritespellInfo(byref spellinfo, cfg := ":magery:config/mage")
	cfg := ReadConfigFile(cfg);
	if (!cfg)
		return 0;
	endif

	var spell_id := spellinfo;
	if(TypeOf(spell_id) == TypeOf( struct{} ) )
		spell_id := spellinfo.id;
	endif
	
	var elem := FindConfigElem(cfg, spell_id);
	if (elem == error)
		return 0;
	endif

	spellinfo := struct{};
	spellinfo.+id 				:= spell_id;
	spellinfo.+name        := GetConfigString( elem, "Name" );
	spellinfo.+powerwords  := GetConfigStringArray( elem, "PowerWords" );
	spellinfo.+alvo        := GetConfigString( elem, "Target" );
	spellinfo.+type        := GetConfigString( elem, "Type" );
	spellinfo.+range       := GetConfigString( elem, "Range" );
	spellinfo.+mana        := GetConfigInt( elem, "Mana" );
	spellinfo.+delay       := GetConfigInt( elem, "Delay" );
	spellinfo.+difficulty  := GetConfigInt( elem, "Difficulty" );
	spellinfo.+description := GetConfigString( elem, "Desc" );
	spellinfo.+script      := GetConfigString( elem, "SpellScript" );
	spellinfo.+reagents    := GetConfigStringDictionary(elem, "RegCost");
	spellinfo.+align       := GetConfigStringDictionary(elem, "Align");
	spellinfo.+flavortext  := GetConfigString( elem, "FlavorText" );
	spellinfo.+cycles      := GetConfigInt( elem, "Cycles" );
	spellinfo.+cooldown    := GetConfigInt( elem, "Cooldown" );
	spellinfo.+circle      := GetConfigInt( elem, "Circle" );
	spellinfo.+charges     := GetConfigInt( elem, "Charges" );
	spellinfo.+radius      := GetConfigInt( elem, "Radius" );
	spellinfo.+powered_move      := GetConfigInt( elem, "PoweredMove" );
	spellinfo.+pericia     := StrReplace(GetConfigString( elem, "Pericia"), " ", "_");
	spellinfo.+targettype     := GetConfigString( elem, "TargetType" );
	if (spellinfo.flavortext == error)
		spellinfo.flavortext := "";
	else
		spellinfo.flavortext := "<I> "  + spellinfo.flavortext + " </I>";
	endif

	return 1;
endfunction

function TNCheckReagents(who, byref spellinfo, item)
	if ( item.isa(POLCLASS_ITEM) && spellinfo.charges )
      spellinfo.charges += Ceil(cdbl(spellinfo.charges) * (cdbl(spellinfo.powered-1) / 10));

      if (AP_GetTrueSkill(who, "Magia_de_{}".format(spellinfo.type)) >= 6)
         spellinfo.charges /= 2;
      endif

		var charges := cint(GetObjProperty(item, "charges"));
		if (charges < spellinfo.charges)
			SendSysMessageEx(who, "Cargas insuficientes na varinha: {} cargas".format(charges), SSM_FAIL);
			return 0;
		endif

		charges -= spellinfo.charges;
      SetName(item, item.desc);
		SetObjProperty(item, "charges", charges);
		return 1;
	endif

	// //sendsysmessage(who,  " " + reagent_costs);
	// var keys := reagent_costs.keys();
	// var amount;
	// foreach reag in keys
	// 	amount := CInt( reagent_costs[reag] );
	// 	if (!amount)
	// 		amount := 1;
	// 	endif
	// 	var objtype := GetObjTypeByName(reag);
	// 	var dictToAmount := dictionary; //variavel pra ser usada na funcao abaixo
	// 	dictToAmount["objtype"] := objtype;
	// 	var amountinpack := AmountInContainer(who.backpack, dictToAmount);
	// 	//sendsysmessage(who, "reag " + reag + " objtype " + objtype + " pack " + amountinpack);
	// 	if (amountinpack  < amount )
	// 		SendSysMessageEx(who, "Voc� n�o possui "+GetObjTypeDesc(objtype, (amount>1)), SSM_FAIL);
	// 		return 0;
	// 	endif
	// 	sleepms(2);
	// endforeach

	// foreach reag in ( keys )
	// 	amount := CInt( reagent_costs[reag] );
	// 	if (!amount)
	// 		amount := 1;
	// 	endif
	// 	ConsumeSubstance(who.backpack, GetObjTypeByName(reag), amount);
	// 	sleepms(2);
	// endforeach

	return 1;
endfunction

function getSpellName(id, cfg := ":magery:config/mage")
	cfg := ReadConfigFile(cfg);
	var elem := FindConfigElem(cfg, id);

	return GetConfigString( elem, "Name" );
endfunction

function MantainSpell(byref info)
	//sendsysmessage(info.mage, " " + info);
	if (info.mage.connected)
		start_script( ":magery:mantainSpell", { info.mage, info.mobserial, info.name, info.duration, info.level  }  ); 
	endif
endfunction

function GetSpellDuration(skill)
	return cint(skill/10)*60 + 300;
endfunction

function WriteMagic(who, spell_id, spell_power)
	var items_bag := EnumerateItemsInContainer(who.backpack, ENUMERATE_IGNORE_LOCKED);
   if (!SearchArrayOfModel("objtype", cint(0x10e4), EnumerateItemsInContainer(who.backpack))) 
		SendSysMessageEx(who, "Voc� precisa de uma faca para entalhar", SSM_FAIL);
		return;
	endif

	var spellinfo := struct{ id := spell_id };

	if (!WriteSpellInfo(spellinfo))
		SendSysMessageEx(who, "Ocorreu um erro ao inscrever a magia. Chame um GM.", SSM_FAIL);
		return;
   elseif (!HaveSkill(who, "Magia_de_{}".format(spellinfo.type)))
		SendSysMessageEx(who, "Voc� n�o tem conhecimento de magia", SSM_FAIL);
		return;
	endif

   if (Ceil(AP_GetTrueSkill(who, "Magia_de_{}".format(spellinfo.type)) / 2) < spellinfo.circle)
		SendSysMessageEx(who, "Voc� n�o tem conhecimento o suficiente para inscrever esta magia.", SSM_FAIL);
		return;
   endif

	SendSysMessageEx(who, "Escolha o objeto para inscrever a magia.", SSM_INFO);
	var targ := Target(who);

	var magic_slot   := GetObjProperty(targ, "magicslot");
	var spell_list   := GetObjProperty(targ, "spell_list");
	if (!spell_list)
		spell_list := array{};
	endif
	var magic_user   := GetObjProperty(targ, "serialid");
	var element      := GetObjProperty(targ, "element");
	var elementpower := GetObjProperty(targ, "elementpower");
	if (!targ.isA(POLCLASS_ITEM))
		SendSysMessageEx(who, "Voc� deve escolher um objeto!", SSM_FAIL);
		return;
	elseif (!magic_slot)
		SendSysMessageEx(who, "Este objeto n�o tem poder magico!", SSM_FAIL);
		return;
	elseif (GetObjProperty(targ, "serialid") && GetObjProperty(targ, "serialid") != who.serial)
		return;
	elseif (spell_list.size()+spell_power > magic_slot)
		SendSysMessageEx(who, "Este objeto n�o possui mais espa�o para magias!", SSM_FAIL);
		SendSysMessageEx(who, "Necess�rio {} slots".format(spell_power), SSM_FAIL);
		return;
	elseif (spell_id in spell_list)
		SendSysMessageEx(who, "O equipamento ja tem essa magia!", SSM_FAIL);
		return;
	elseif(magic_user && magic_user != who.serial)
		SendSysMessageEx(who, "Voc� n�o pode gravar em itens de outra pessoa!", SSM_FAIL);
		return;
	endif

	var loc := array{who.x, who.y};
	for i := 1 to 2
		if (who.x != loc[1] && who.y != loc[2])
			SendSysMessageEx(who, "Voc� se moveu e parou de gravar no objeto", SSM_FAIL);
			return;
		endif
		PrintText(who, "*inscrevendo em {}*".format(targ.desc));
		PlaySoundEffect(who, 0x002b);
		sleep(2);
	endfor

	if ((spellinfo.type != "Comando" && spellinfo.type != element))
		SendSysMessageEx(who, "Os hieroglifos n�o reagem aos elementos da varinha", SSM_FAIL);
		return;
   elseif (spellinfo.circle > elementpower)
		SendSysMessageEx(who, "Os hieroglifos s�o muito poderosos para essa varinha", SSM_FAIL);
		return;
	endif

	var dice_roll := rollAttrDice(who, INTELLIGENCE) + GetSkillBonus(who, "Magia_de_{}".format(spellinfo.type));
	if (dice_roll <= spellinfo.difficulty)
		SendSysMessageEx(who, "Parece que voc� errou durante a c�pia. O espa�o ficou inutilizado.", SSM_FAIL);
		spell_list.append(0);
		SetObjProperty(targ, "spell_list", spell_list);
      SetObjProperty(targ, "magicwho", array{ who.serial });
      SetName(targ, targ.name);
		return;
	endif

	ExpGain(who, spellinfo.difficulty+3, dice_roll);
   for i := 1 to spell_power
	   spell_list.append(spell_id);
   endfor
	SetObjProperty(targ, "serialid", who.serial);
	SetObjProperty(targ, "spell_list", spell_list);
	SetObjProperty(targ, "magicwho", array{ who.serial });
	SetName(targ, targ.desc);
	SendSysMessageEx(who, "Voc� inscreveu com sucesso a magia na arma.", SSM_INFO);
endfunction

function dispel(targ);
	var timer_list := TS_GetTimerList(targ);
	var names := array;

	foreach timer in ( timer_list.keys() )
		timer := lower(timer);
		if (timer == "agility" )
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "strength");
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "intelligence")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "velocidade")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "PhysicalResist")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "bless")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "phmod")
			TS_LowerDuration(targ, timer, -1);
		elseif (timer == "fimod")
			TS_LowerDuration(targ, timer, -1);
		elseif (timer == "comod")
			TS_LowerDuration(targ, timer, -1);
		elseif (timer == "enmod")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "pomod")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "disfarce")
			TS_LowerDuration(targ, timer, -1);
		elseif ( timer == "gloria")
			TS_LowerDuration(targ, timer, -1);
		elseif (timer == "desviarprojetil")
			TS_LowerDuration(targ, timer, -1);
		endif
		
	endforeach
	
	EraseObjProperty(targ, "tirocerteiro");
	EraseObjProperty(targ, "evasao");
endfunction 

function SetClericSpellUsed(mobile, spellinfo)
	var cleric_list := GetObjProperty(mobile, "cleric_list");
	foreach spell in cleric_list
		if (spell.id == spellinfo.id && !spell.used)
			spell.used := 1;
			break;
		endif
		sleepms(2);
	endforeach

	SetObjProperty(mobile, "cleric_list", cleric_list);
endfunction

function CreateRandomGliph(byref gliphs, x, y, z)
	var gliph_graphics := array{0x485F, 0x4844, 0x4841, 0x483B, 0x4862, 0x4865, 0x4868, 0x487A, 0x487D, 0x485C, 0x485F};
	var gliph := CreateItemAtLocation(x, y, z, 0x790d);
	
	gliph.graphic := gliph_graphics.randomentry();
	
	if (typeof(gliphs) == "Array")
		gliphs.append(gliph);
	else
		return gliph;
	endif
endfunction

function SetClericHeal(who, mobile)
   if (!mobile.isA(POLCLASS_MOBILE))
      mobile := who;
   endif
   if (mobile == who)
      SendSystemReport(mobile, "Ben��o de Sacrif�cio retornou para voc�");
   else
      SendSystemReport(mobile, "{} direcionou ben��os para voc�".format(who.name));
   endif

   who.addBuff(cint(1069), 0, cint(1075833), cint(1075834), cAscZ("Curando {}".format(mobile.name)));
   SetObjProperty(who, "cleric_spell", mobile.serial);
endfunction
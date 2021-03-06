// $Id: advanceCheck.inc 386 2005-10-03 00:22:39Z muaddiblsd $

/*===============================================================
* Current Version
* ADVANCECHECK.INC - v1.0
* Updated 9/28/2005 8:29PM
*
* -- Revision v1.0 --
* Austin:
*  Created include file
===============================================================*/

use uo;
use os;
use util;

include ":charactercreation:characters";
include ":timeUtils:time";

function GetCharAge(character, factor := "days")
	var create_date := cint(GetObjProperty(character, "createdate"));
	var days        := GetTimeBetween(cint(create_date), GetTime(), factor);
	return days;
endfunction
  
function GetExpCap(who)
	if (who.npctemplate)
		return 0;
	endif

   var extra_cap := Cint(GetObjProperty(who, "extracap"));
	var char_age := GetCharAge(who, "weeks") +1;
	var char_level := who.getLevel() - 1; //hack
	var result := ((char_age*2000) + extra_cap) - (char_level*500);
	return result;
endfunction

/*
 * ExpGain(mobile, stat_name, stat_mult)
 *
 * Purpose
 * Does an advancement check on a stat.
 *
 * Parameters
 * mobile:	Mobile to do the skill advancement check on.
 * skill_name:	Name of the skill to check.
 * award_diff:	Difficulty to use for advancement award.
 * pass_chance:	Chance of getting a gain check.
 * 
 *
 * Return value
 * Returns 1
 *
 */

function SkillDbgMsg(mobile, msg)
	if (mobile.cmdlevel > 4)
		SendSysMessage(mobile, msg);
	endif
endfunction

function ExpGain(mobile, difficulty, value := 0)
	if ( GetObjProperty(mobile, "NoGains") ) //exp gain blocked
		SkillDbgMsg(mobile, "Flag NoGains setada.");
		return 0;
	endif

   var incr_gain := 10;
  if ( GetExpCap(mobile) < mobile.Getexp())
  		SkillDbgMsg(mobile, "Chegou no maximo do limite por tempo: {}".format(GetExpCap(mobile)));
		incr_gain += 200;
   endif
  
	// var settings := AP_GetSettingsCfgElem("Skills");
	//verifica se ja n�o recebeu exp em 5 segundos
	var next_check := CInt(GetObjProperty(mobile, "#LastSkillGain")) + incr_gain;
	if ( next_check > ReadGameClock() )
	//	printtextabove(mobile, "n�o posso subir skill agora");
		SkillDbgMsg(mobile, "N�o pode subir skill ainda. Next: " + next_check + " Current: " + ReadGameClock()  );
		return 0;
	endif

	//calcula a chance de passar
	var up_amount := 0;
	if (difficulty == "major")
		up_amount := 4;
	elseif (difficulty == "minor")
		up_amount := 2;
	elseif (TypeOf(difficulty) == TypeOf( 1 ))
		var diff := difficulty - value;
		if (diff <= -1)
			up_amount := 1;
		elseif(diff >= 10)
			up_amount := 2;
		elseif(diff >= 6)
			up_amount := 3;
		elseif(diff >= 4)
			up_amount := 4;
		else
			up_amount := 5;
		endif
	else
		SysLog("Erro em difficulty para ExpGain: {}".format(difficulty));
	endif

   var account := FindAccount(mobile.acctname);
   if (account && account.GetProp("expboost"))
      if (account.GetProp("expboost") <= mobile.getlevel())
         account.EraseProp("expboost");
      else
         up_amount := up_amount * 3;
      endif
   elseif (mobile.guild.getProp("leader") == mobile.serial)
      up_amount := up_amount * 2;
   endif

	//guarda as skills dos npcs tamados
	if (mobile.npctemplate && GetObjProperty(mobile, "owner") != error)
      mobile.setExp(1);
		// TODO: Tamed exp gain
	elseif (up_amount > 0)
		var exp := mobile.GetExp();
		mobile.SetExp(exp+up_amount);
		CheckExpUp(mobile);
      var pnt_amt := cint(GetObjProperty(mobile, "points_received"))+1;
      var total_exp := mobile.getTotalExp();
      if (total_exp >= (pnt_amt * 120))
         mobile.SetTalentPoints(1);
         SetObjProperty(mobile, "points_received", pnt_amt);
      endif
	endif

	SetObjProperty(mobile, "#LastSkillGain", ReadGameClock());
	return 1;
endfunction

function CheckExpUp(who)
	var level := who.getLevel();
	if (level < 10)
		var maxexp := who.getMaxExp(); //starts at 2000
		var exp := who.GetExp();
      
		if (exp < maxexp)
			return;
		endif
		who.StoreExp(maxexp);
		who.setLevel(level+1);
		IncreaseVitalPoints(who);
		var levelup_cfg := ReadConfigFile(":attributes:config/levelup");
      levelup_cfg := levelup_cfg[level+1];
		if (cint(levelup_cfg.AttributePoints))
			who.SetPoints(1);
		endif
	endif
endfunction 

function IncreaseVitalPoints(who)
	var chardata  := GetObjProperty(who, "chardata");

	var increase_hitpoint := RandomDiceRoll("1d5+2") + AP_GetStat(who, CONSTITUTION, RETURN_BONUS);
	var increase_manapoint := RandomDiceRoll("1d5+2") + AP_GetStat(who, INTELLIGENCE, RETURN_BONUS);
	var increase_stampoint := 1 + AP_GetStat(who, DEXTERITY, RETURN_BONUS);

   var duro_de_matar := chardata.carac["Duro de Matar"];
   if (duro_de_matar) increase_hitpoint += duro_de_matar; endif

   var acumulador_mana := chardata.carac["Acumulador de Mana"];
   if (acumulador_mana) increase_manapoint += acumulador_mana; endif

	chardata.hitpoints  += cint(increase_hitpoint);
	chardata.manapoints += increase_manapoint;
	chardata.stampoints += increase_stampoint;

	SetObjProperty(who, "chardata", chardata);
   RecalcVitals(who);
endfunction

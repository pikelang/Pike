#pike __REAL_VERSION__


//! Portuguese language locale

// $Id$

inherit "abstract";

constant name = ""; // FIXME
constant english_name = "portuguese";
constant iso_639_1 = "pt";
constant iso_639_2 = "por";
constant iso_639_2B = "por";

constant aliases = ({ "pt", "por", "port", "portuguese" });
 
constant months = ({
  "Janeiro", "Fevereiro", "Março", "Abril", "Maio",
  "Junho", "Julho", "Agosto", "Setembro", "Outubro",
  "Novembro", "Dezembro" });
 
constant days = ({
  "Domingo","Segunda Feira","Terça Feira","Quarta Feira",
  "Quinta Feira","Sexta Feira","Sábado" });
 
string ordered(int i)
{
    return i+"º";
}
 
string date(int timestamp, string|void m)
{
  mapping t1=localtime(timestamp);
  mapping t2=localtime(time(0));

  if(m=="full")
    return ctime(timestamp)[11..15]+", "+
           month(t1["mon"]+1) + " the "
           + ordered(t1["mday"]) + ", " +(t1["year"]+1900);

  if(m=="date")
    return month(t1["mon"]+1) + " the "  + ordered(t1["mday"])
      + " no ano de " +(t1["year"]+1900);

  if(m=="time")
    return ctime(timestamp)[11..15];

  // !m

  if(t1["yday"] == t2["yday"] && t1["year"] == t2["year"])
    return "hoje, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]+1 == t2["yday"] && t1["year"] == t2["year"])
    return "ontem, "+ ctime(timestamp)[11..15];
  
  if(t1["yday"]-1 == t2["yday"] && t1["year"] == t2["year"])
    return "amanhã, "+ ctime(timestamp)[11..15];
  
  if(t1["year"] != t2["year"])
    return (month(t1["mon"]+1) + " " + (t1["year"]+1900));

  return (month(t1["mon"]+1) + " " + ordered(t1["mday"]));
}
 
 
string number(int num)
{
  if(num<0)
    return "menos "+number(-num);
  switch(num)
  {
   case 0:  return "";
   case 1:  return "um";
   case 2:  return "dois";
   case 3:  return "tres";
   case 4:  return "quatro";
   case 5:  return "cinco";
   case 6:  return "seis";
   case 7:  return "sete";
   case 8:  return "oito";
   case 9:  return "nove";
   case 10: return "dez";
   case 11: return "onze";
   case 12: return "doze";
   case 13: return "treze";
   case 14: return "catorze";
   case 15: return "quinze";
   case 16: return "dezesseis";
   case 17: return "dezessete";
   case 18: return "dezoito";
   case 19: return "dezenove";
   case 20: return "vinte";
   case 30: return "trinta";
   case 40: return "quarenta";
   case 50: return "cinquenta";
   case 60: return "sessenta";
   case 70: return "setenta";
   case 80: return "oitenta";
   case 90: return "noventa";
 
   case 21..29: 
   case 31..39: 
   case 41..49:
   case 51..59: 
   case 61..69: 
   case 71..79: 
   case 81..89: 
   case 91..99:  
     return number((num/10)*10)+ " e " +number(num%10);
 
   case 100..199: return "cento e "+number(num%100);
   case 200..299: return "duzentos e "+number(num%100);
   case 300..399: return "trezentos e "+number(num%100);
   case 400..499: return "quatrocentos e "+number(num%100);
   case 500..599: return "quinhentos e "+number(num%100);
   case 600..699: return "seiscentos e "+number(num%100);
   case 700..799: return "setecentos e "+number(num%100);
   case 800..899: return "oitocentos e "+number(num%100);
   case 900..999: return "novecentos e "+number(num%100);
 
   case 1000..1999: return "mil "+number(num%1000);
   case 2000..999999: return number(num/1000)+" mil "+number(num%1000);
 
   case 1000000..1999999: 
     return "um milhão "+number(num%1000000);
 
   case 2000000..999999999: 
     return number(num/1000000)+" milhões "+number(num%1000000);
 
   default:
    return "muito!!";
  }
}

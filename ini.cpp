//
#pragma warning(disable: 4786)

#include <vector>
#include <iostream>
#include "bnflite.h"

using namespace bnf;
using namespace std;


/* ini file configuration:
[section_1]
var1=value1
var2=value2

[section_2]
var1=value1
var2=value2
*/


const char* ini = // this is example of ini-like configuration file
"; last modified 1 April 2001 by John Doe\n"
" [ owner ]\n"
"name=John Doe\n\n"
"organization=Acme Widgets Inc.\n"
"\n"
"[database]\n \n"
"; use IP address in case network name resolution is not working\n"
"server=192.0.2.62   \n"  
"port= 143\n"
"file=\"payroll.dat\"\n";


struct Section
{
     string name; 
     vector< pair< string, string> > value;
     Section(const char* name, size_t len) :name(name, len) {}
};
vector <struct Section> Ini;  // ini-file configuration container 

// example for custom interface instead of "typedef Interface<int> Gen;"
class Gen :public  Interface<>
{
public:
    Gen(const Gen& ifc, const char* text, size_t length, const char* name)
        :Interface<>(ifc, text, length, name){}
    Gen(const char* text, size_t length,  const char* name)
        :Interface<>(text, length, name){}
    Gen(const Gen& front, const Gen& back)
        :Interface<>(front, back) {}
    Gen() :Interface<>() {};
};


static bool printMsg(const char* lexem, size_t len)
{   // debug function
    printf("Debug: %.*s;\n", len, lexem);
    return true;
}


Gen DoSection(vector<Gen>& res)
{   // save new section, it is 2nd lexem in section Rule in main
    Ini.push_back(Section(res[1].text, res[1].length));
    return Gen(res.front(), res.back());
}

Gen DoValue(vector<Gen>& res)
{   // save new entry: 4th lexem - name and 7th lexem is property value
   int i = res.size();
   if (i > 2 )
    Ini.back().value.push_back(make_pair(
       string(res[0].text, res[0].length), string(res[2].text, res[2].length)));
   return Gen(res.front(), res.back());
}

static void Bind(Rule& section, Rule& entry)
{
    Bind(section, DoSection);
    Bind(entry, DoValue);
}

// example of custom parser
class ini_parser : public _Parser<Gen>
{
public:
    const char* zero_parse(const char* ptr)
    {   // skip ini file comments
        if (*ptr ==';' ||  *ptr =='#')
            while (*ptr != 0)
                if( *ptr++ == '\n')
                    break;
        return ptr;
    }
};


int main()
{

    Token space(" \t");  // space and tab are grammar part in ini files
    Token delimiter(" \t\n\r");     // consider new lines as grammar part too
    Token name("_.,:(){}-#@&*|");  // start declare with special symbols
	name.Add('0', '9'); // appended numeric part
	name.Add('a', 'z'); // appended alphabetic lowercase part
	name.Add('A', 'Z'); // appended alphabetic capital part
    Token value(1,255); value.Remove("\n");

    Lexem Name = 1*name;
    Lexem Value = *value;
    Lexem Equal = *space + "=" + *space;
    Lexem Left  = *space + "[" + *space;        // bracket
    Lexem Right  = *space + "]" + *space;
    Lexem Delimiter  = *delimiter;

    Rule Item = Name + Equal + Value + "\n";
    Rule Section = Left + Name + Right + "\n";
    Rule Inidata = Delimiter + *(Section + Delimiter + *(Item + Delimiter)) ;


    Bind(Section, Item);

    const char* tail = "";
    ini_parser myParser;


    int tst = Analyze(Inidata, ini, myParser);
    myParser.Get_tail(&tail);
    if (tst > 0)
        cout << "Section read:" << Ini.size();
    else
        cout << "Parsing errors detected, status = " << hex << tst << endl
         << "stopped at: " << tail << endl;


    for (vector<struct Section>::iterator j = Ini.begin(); j != Ini.end(); ++j) {
        cout << endl << "Section " << j->name << " has " << (*j).value.size() << " values: "; 
        for (vector<pair<string, string> >::iterator i = j->value.begin(); i != j->value.end(); ++i) {
            cout << i->first << "="  << i->second <<"; ";
        }
    }

	return  0; 
}



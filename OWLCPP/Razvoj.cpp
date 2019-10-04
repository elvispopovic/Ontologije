#include <iostream>
#include <string>

#include "boost/filesystem.hpp"
#include "boost/foreach.hpp"
#include "boost/program_options.hpp"
#include "boost/range.hpp"

#include "factpp/Kernel.hpp"
#include "owlcpp/rdf/triple_store.hpp"
#include "owlcpp/io/input.hpp"
#include "owlcpp/io/catalog.hpp"
#include "owlcpp/logic/triple_to_fact.hpp"

using namespace std;
namespace oc = owlcpp;
namespace bpo = boost::program_options;
namespace bfs = boost::filesystem;

void ispisiTrojku(string t1, string t2, string t3)
{
	cout << '\"' << t1 << "\"\t\"" << t2 << "\"\t\"" << t3 << "\"\t" << endl;
}
void ispisiEntitet(string s)
{
	cout << '\"' << s << "\"" << endl;
}

int main(int argc, char* argv[]) 
{
	/* parsiranje opcija i parametara */
	bpo::options_description od;
	od.add_options()
	("pomoc,h", "Poruka pomoci")
	("datoteka", bpo::value<string>(), "Unos OWL datoteke")
	("include,i", bpo::value<vector<string> >()->zero_tokens()->composing(), "Ukljucivanje")
	("trojke,t", bpo::bool_switch(), "Ispis trojki")
	("klase,k", bpo::bool_switch(), "Ispis klasa")
	("svojstva,s", bpo::bool_switch(), "Ispis svojstava")
	("individuals,n", bpo::bool_switch(), "Ispis ogranicenja")
	("dijagnoza,d", bpo::bool_switch(), "Provjera gresaka u zakljucivanju nakon svakog aksioma ")
	("labavo,l", bpo::bool_switch(), "Labavo, ne-striktno parsiranje")
	("uspjeh,u", bpo::bool_switch(), "Vraca 1 ako ontologija nije konzistentna");
	
	bpo::positional_options_description pod;
	pod.add("datoteka", -1);
	bpo::variables_map vm;
	store(bpo::command_line_parser(argc, argv).options(od).positional(pod).run(), vm);
	notify(vm);

	if( !vm.count("datoteka") || vm.count("pomoc") )
	{
		cout << "Program za rad sa OWL ontologijama." << endl;
		cout << "Koristenje:" << endl;
		cout << "OntoProg [-i[staza]] [-c] <ontologija.owl>" << endl;
		cout << od << endl;
		return !vm.count("pomoc");
	}
	
	/* Trojke */
	oc::Triple_store trojke;
	const bfs::path datoteka = vm["datoteka"].as<string>();
	try
	{
		if( vm.count("include") )
		{
			oc::Catalog katalog;
			vector<string> const& vin = vm["include"].as<vector<string> >();
			if( vin.empty() )
				add(katalog, datoteka.parent_path(), true);
			else
				BOOST_FOREACH(string const& p, vin) add(katalog, p, true);
		}
		else
			load_file(datoteka, trojke);
		if( vm["trojke"].as<bool>() )
		{
			cout << "Ucitano je " << trojke.map_triple().size() << " trojki, " << trojke.map_node().size() 
				<< " cvorova, " << trojke.map_ns().size() << " imenskih IRI-a." << endl;
			BOOST_FOREACH( owlcpp::Triple const& t, trojke.map_triple() ) 
				ispisiTrojku(to_string(t.subj_, trojke), to_string(t.pred_, trojke), to_string(t.obj_, trojke));           	
		}
		
		if( vm["svojstva"].as<bool>() )
		{
			owlcpp::Triple_store::query_b<0,1,1,0>::range r = trojke.find_triple( owlcpp::any,
               	owlcpp::terms::rdf_type::id(), owlcpp::terms::owl_ObjectProperty::id(), owlcpp::any);
			cout << "Pronadjeno je " << r.size() << " svojstava." << endl;
			BOOST_FOREACH( owlcpp::Triple const& t, r) 
				ispisiEntitet(to_string(t.subj_, trojke));  
			
		}	
		
		if( vm["klase"].as<bool>() )
		{
			owlcpp::Triple_store::query_b<0,1,1,0>::range r = trojke.find_triple( owlcpp::any,
               	owlcpp::terms::rdf_type::id(), owlcpp::terms::owl_Class::id(), owlcpp::any);
			cout << "Pronadjeno je " << r.size() << " klasa." << endl;
			BOOST_FOREACH( owlcpp::Triple const& t, r ) 
			{
				ispisiEntitet(to_string(t.subj_, trojke));  
			}
		}	
		
		if( vm["individuals"].as<bool>() )
		{
			owlcpp::Triple_store::query_b<0,1,1,0>::range r = trojke.find_triple( owlcpp::any,
               	owlcpp::terms::rdf_type::id(), owlcpp::terms::owl_NamedIndividual::id(), owlcpp::any);
			cout << "Pronadjeno je " << r.size() << " individua." << endl;
			BOOST_FOREACH( owlcpp::Triple const& t, r ) 
				ispisiEntitet(to_string(t.subj_, trojke));  
			
		}	
	}
	catch(...)
	{
		cerr << "Greska prilikom ucitavanja trojki iz datoteke." << endl;
		cout << "Poruka boosta: " << boost::current_exception_diagnostic_information() << endl;
		return 1;
	}
	
	/* Zakljucivac FaCT++ */
	try
	{
		ReasoningKernel kernel;
		const std::size_t n = submit(trojke, kernel, vm["labavo"].as<bool>(), vm["dijagnoza"].as<bool>());
		cout << "Generirano je " << n << " aksioma." << endl;
		const bool konzistentna = kernel.isKBConsistent();
		if(konzistentna)
			cout << "Ontologija je konzistentna." << endl;
		else
			cout << "Ontologija nije konzistentna." << endl;
		return vm["uspjeh"].as<bool>() && ! konzistentna;
	}
	catch(...)
	{
		cout << "Greska prilikom upotrebe zakljucivaca." << endl;
		cout << "Poruka boosta: " << boost::current_exception_diagnostic_information() << endl;
		return 1;
	}
	
}
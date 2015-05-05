#include "dbxml/DbXml.hpp"
#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"

namespace po = boost::program_options;
namespace db = DbXml;
namespace fs = boost::filesystem;

#include <iostream>
#include <iterator>

using namespace std;

// Global vars
fs::path data_dir;
fs::path case_dir;
fs::path master_lists_dir;
fs::path container_names_dir;
fs::path container_cases_dir;

bool simulate = false;
bool verbose = false;

// Imports an XML file into the specifiedd container
void ImportXML(fs::path file, db::XmlContainer container, db::XmlManager manager) {
    db::XmlUpdateContext context = manager.createUpdateContext();
    db::XmlInputStream *stream = manager.createLocalFileInputStream(file.string());
    
    if(verbose || simulate)
        cout << "Importing " << file.filename().string() << "\n";
    
    if(simulate) return;
    
    try {
        container.putDocument(file.filename().string(), stream, context, 0);
    } catch (DbXml::XmlException &xe) {
        printf ("%s\n", xe.what());
        // Error handling goes here
    } catch (std::exception &e) {
        // Error handling goes here
    }
}

// Import all cases into the cases container
void ImportCases(fs::directory_iterator case_dir, db::XmlManager manager, fs::path container_cases_dir) {
    // Create a new container
    db::XmlContainer container;
    
    if(!simulate)
        container = manager.createContainer(container_cases_dir.string());
    
    // Loop through all cases
    for(fs::directory_entry& case_file : case_dir) {
        if(case_file.path().extension() == ".xml")
            ImportXML(case_file.path(), container, manager);
    }
    
    // Add indices
    db::XmlUpdateContext context = manager.createUpdateContext();
    container.addIndex("tei", "person/@xml:id", "node-attribute-equality-string", context);
    container.addIndex("tei", "TEI/@xml:id", "unique-node-attribute-equality-string", context);
}

void ImportNames(vector<fs::path> name_files, db::XmlManager manager, fs::path container_names_dir) {
    db::XmlContainer container;
    
    if(!simulate)
        container = manager.createContainer(container_names_dir.string());
    
    for(fs::path name_file : name_files){
        ImportXML(name_file, container, manager);
    }
    
    // Add indices
    db::XmlUpdateContext context = manager.createUpdateContext();
    container.addIndex("tei", "person/@xml:id", "unique-node-attribute-equality-string ", context);
    container.addIndex("tei", "TEI/@xml:id", "unique-node-attribute-equality-string", context);
    container.addIndex("tei", "link/@sameAs", "node-attribute-equality-string", context);
}

int main(int ac, char* av[])
{
    try {
        // Initialize program_options boost library
        po::options_description desc("Import the casebooks data into two db files. Allowed options");
        desc.add_options()
            ("help,h", "Produce this help message")
            ("data-dir,d", po::value<string>(), "Data directory of the Casebooks project")
            ("simulate,s", "Simulate (don't alter anything)")
            ("verbose,v","Show progress")
        ;
        
        po::variables_map vm;
        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);
        
        // Display help if needed
        if (vm.count("help")) {
            cout << desc << "\n";
            return 0;
        }
        
        if (vm.count("simulate")) {
            simulate = true;
        }
        
        if (vm.count("verbose")) {
            verbose = true;
        }
        
        // Initialize directories
        data_dir            = vm["data-dir"].as<string>();
        case_dir            = data_dir/"cases";
        master_lists_dir    = data_dir/"master-lists";
        container_names_dir = master_lists_dir/"names.dbxml";
        container_cases_dir = case_dir/"cases.dbxml";

        // Declare some sort of manager
        db::XmlManager *myManager = new db::XmlManager(db::DBXML_ALLOW_EXTERNAL_ACCESS);
        
        // Get all case files and import them
        fs::current_path(case_dir);
        ImportCases(fs::directory_iterator(case_dir), *myManager, container_cases_dir);

        cout << "Done importing cases!" << "\n";
        
        // Import the name files
        fs::current_path(master_lists_dir);
        vector<fs::path> name_files =
            {
                master_lists_dir/"names-forman-only.xml",
                master_lists_dir/"names-napier-only.xml",
                master_lists_dir/"names-both-forman-napier.xml",
                master_lists_dir/"names-hands.xml"
            };
        
        ImportNames(name_files, *myManager, container_names_dir);
        
        cout << "Done importing names!";
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
    }
    
    return 0;
}



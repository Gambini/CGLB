#pragma once


/**
 * At runtime, will generate documentation of all of the types/functions bound
 * through this library to a file named in the CGLB_BINDING_DOC_FILENAME file
 * through std::fstream. 
 *
 * If std::fstream is not available, place an issue on
 * the github repo (https://github.com/Gambini/CGLB), and I will make a more
 * generic read|write abstraction available.
 *
 * Defaults to undefined.
 */
#define CGLB_GENERATE_BINDING_DOC
#define CGLB_BINDING_DOC_FILENAME "cglb_binding_doc.txt"

##############################################################
#
#   RDF_Creator.py
#
#
#       Python based file to genearte the RDF files required
#       for MR ACCESS to correlate memory locations to
#       register names.
#
##############################################################

import os
import sqlite3
import logging

scriptDir = os.getcwd()

CW_Version = "CW4NET_v2016.01"
DB_Dir = "C:/Freescale/" + CW_Version + "/CW_ARMv8/Config/SoCs/registers/"

# Create logger
logger = logging.getLogger(__name__)
# Set up the logger        
logger.setLevel(logging.DEBUG) 


##  Setup log file configuration 
frmt = logging.Formatter('%(name)s - %(lineno)d - %(levelname)s - %(message)s')
# Create log handler for logging messages.  Log file created
# each time vs appended.
lh = logging.FileHandler('RDF_Creator_debug.log', mode='w', delay=True)
lh.setFormatter(frmt)   

# Assign the handler to the log
logger.addHandler(lh) 
# Set the logging level for DEBUG messages
lh.setLevel(logging.DEBUG)
 


##############################################################


def main():

    PROCS = get_processor_file_list()
    
                
    for file in PROCS:
        logger.debug(" ")
        logger.debug("Processing for %s" %file)
        inputFile = DB_Dir + file
        outputFile = scriptDir + "\\" + file.split(".")[0] + ".rdf"
        
        generate_rdf_file( inputFile, outputFile)
        logger.debug("Processing for %s completed" %file)


##############################################################
        
def get_processor_file_list():
    
    # Create empty list to hold the register database files
    PROCS = []
    
    # Get a listing of all of the current register database files
    # with the exception of the simulator based ones.
    for file in os.listdir(DB_Dir):
        if not ("simulator") in file:
            # Remove the .sqlite extension for the proc list
            PROCS.append(file)
    return PROCS        
            


def generate_rdf_file(inputFile, outputFile):

    currentProc = os.path.basename(inputFile).split(".")[0]
    OFFSET_WARNING = "FALSE"

    conn = sqlite3.connect(inputFile)
    
    cursor = conn.cursor()
    
    sql = "SELECT * from RegGroups where CollectionId = '2'"
    cursor.execute(sql)

    GROUPDETAILS = cursor.fetchall()
    
    #  Make sure there is data in the database
    
    if GROUPDETAILS:
        
        RDF_file_ptr = open(outputFile, 'w')

        print("     Processing: %s" %os.path.basename(inputFile))
        
        # For the ARMv8 Procs the regs are all offset from 0x0
        
        RDF_file_ptr.write("MAP BASE 0x0\n")
        RDF_file_ptr.write("\n")
        
        for REGISTERBLOCK in GROUPDETAILS:
            
            block = REGISTERBLOCK[2]
        
            RDF_file_ptr.write("BLOCK NAME %s\n" %block)
            logger.debug("   Parsing Block: %s - %s" %( currentProc, block))
           
            sql = "SELECT * from RegGroups where Name = \"%s\"" %block
            
            
            cursor.execute(sql)
            GROUPDETAILS = cursor.fetchall()
            
            groupID = int([x[0] for x in GROUPDETAILS][0])
            
            #print GROUPDETAILS
            #print "the GroupID is %s" %groupID
            
            
            sql = "SELECT * from Registers where GroupId = %s" %groupID
            
            cursor.execute(sql)
            for registerData in cursor.fetchall():
                registerName = registerData[3]
                registerSize = registerData[4]
                registerAccess = registerData[5]
                registerLocation = hex(registerData[6])
                
                # Some of the data base files are missing the main block offset
                # so we can fix the ones we know about by adding this block offset 
                # to the offset for each register
                
                if currentProc == "LS1012A" and block == "SerDes":
                    SerDes_Base = "0x1EA0000"
                    registerLocation = hex(int(SerDes_Base, 0) + int(registerLocation, 0))
                    logger.debug("        Fixing offset for %s - %s!" %(registerName, registerLocation))
                     
                
                RDF_file_ptr.write("   %10s  %2s  %3s  %s\n" %(registerLocation, registerSize, registerAccess, registerName))
                
                # check to see if the offset is a valid length that should include
                # the base block offset as well as the register offset within
                # that block
                if len(registerLocation) <= 7:
                    OFFSET_WARNING = "TRUE"
                    logger.debug("        %s has possible incomplete offset of %s!" %(registerName, registerLocation))
            
            # If there was an offset warning let the user know on the console        
            if OFFSET_WARNING == "TRUE":
                print("         At least one reg in %s may not contain full block offset!" %block)
                OFFSET_WARNING = "FALSE"
                        
            RDF_file_ptr.write("BLOCK END\n")
            RDF_file_ptr.write("\n")
                       
        RDF_file_ptr.close()
        
    else:
        # There was a sqlite file but no data for the peripheral blocks
        # within it.
        
        print("No register data for %s"  %os.path.basename(inputFile))
        logger.debug("")
        logger.debug("No register data for %s"  %os.path.basename(inputFile))
        logger.debug("Skipping....")
        
    print("\n")    

################################


main()

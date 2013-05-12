#! /usr/bin/python
#
# join
#   Joing pages from a a collection of PDF files into a single PDF file.
#
#   join [--output <file>] [--shuffle] [--verbose]"
#
#   Parameter:
#
#   --shuffle
#	Take a page from each PDF input file in turn before taking another from each file.
#	If this option is not specified then all of the pages from a PDF file are appended
#	to the output PDF file before the next input PDF file is processed.
#
#   --verbose
#   Write information about the doings of this tool to stderr.
#
import sys
import os
import getopt
import tempfile
import shutil
from CoreFoundation import *
from Quartz.CoreGraphics import *

verbose = False

def createPDFDocumentWithPath(path):
	global verbose
	if verbose:
		print "Creating PDF document from file %s" % (path)
	return CGPDFDocumentCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, path, len(path), False))

def writePageFromDoc(writeContext, doc, pageNum):

	global verbose
	page = CGPDFDocumentGetPage(doc, pageNum)
	if page:
		mediaBox = CGPDFPageGetBoxRect(page, kCGPDFMediaBox)
		if CGRectIsEmpty(mediaBox):
			mediaBox = None
			
		CGContextBeginPage(writeContext, mediaBox)
		CGContextDrawPDFPage(writeContext, page)
		CGContextEndPage(writeContext)
		if verbose:
			print "Copied page %d from %s" % (pageNum, doc)

def shufflePages(writeContext, docs, maxPages):
	
	for pageNum in xrange(1, maxPages + 1):
		for doc in docs:
			writePageFromDoc(writeContext, doc, pageNum)
				
def append(writeContext, docs, maxPages):

	for doc in docs:
		for pageNum in xrange(1, maxPages + 1) :
			writePageFromDoc(writeContext, doc, pageNum)

def main(argv):

	global verbose

	# The PDF context we will draw into to create a new PDF
	writeContext = None

	# If True then generate more verbose information
	source = None
	shuffle = False
	
	# Parse the command line options
	try:
		options, args = getopt.getopt(argv, "o:sv", ["output=", "shuffle", "verbose"])

	except getopt.GetoptError:
		usage()
		sys.exit(2)

	for option, arg in options:

		if option in ("-o", "--output") :
			if verbose:
				print "Setting %s as the destination." % (arg)
			writeContext = CGPDFContextCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, arg, len(arg), False), None, None)

		elif option in ("-s", "--shuffle") :
			if verbose :
				print "Shuffle pages to the output file."
			shuffle = True

		elif option in ("-v", "--verbose") :
			print "Verbose mode enabled."
			verbose = True

		else :
			print "Unknown argument: %s" % (option)
	
	if writeContext:
		# create PDFDocuments for all of the files.
		docs = map(createPDFDocumentWithPath, args)
		
		# find the maximum number of pages.
		maxPages = 0
		for doc in docs:
			if CGPDFDocumentGetNumberOfPages(doc) > maxPages:
				maxPages = CGPDFDocumentGetNumberOfPages(doc)
	
		if shuffle:
			shufflePages(writeContext, docs, maxPages)
		else:
			append(writeContext, docs, maxPages)
		
		CGPDFContextClose(writeContext)
		del writeContext
		#CGContextRelease(writeContext)
    
def usage():
	print "Usage: join [--output <file>] [--shuffle] [--verbose]"

if __name__ == "__main__":
	main(sys.argv[1:])

    
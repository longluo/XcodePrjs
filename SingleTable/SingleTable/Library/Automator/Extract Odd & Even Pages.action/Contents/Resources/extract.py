#! /usr/bin/python
#
# extract
#   Pull pages from a PDF files and place it into another PDF file.
#
#   extract <input_file>] <output_file> [--odd] [--even] [--slice <slice>] [--preview] [--verbose]
#
#   Parameter:
#
#   --odd
#   Copy the odd pages from the source PDF to the new PDF.
#
#   --even
#   Copy the even pages from the source PDF to the new PDF.
#
#
#   --slice <python-slice>
#   Using the Python syntax for a slice, copy a set of pages from the
#   source PDF to the new PDF. For example, --slice [-1] will copy the last
#   page from a PDF. --slice [1] copies the second page (slices are 0 based).
#   --slice [1:-1] copies all but the first page.
#
#   --preview
#   Open the newly created file in Preview. When processed, this option finishes
#   the creation of the new PDF.
#
#   --verbose
#   Write information about the doings of this tool to stderr.
#
import sys
import os
import getopt
import tempfile
import shutil
from Quartz.CoreGraphics import *

verbose = False

def createPDFDocumentWithPath(path):
	global verbose
	if verbose:
		print "Creating PDF document from file %s" % (path)
	return CGPDFDocumentCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, path, len(path), False))

def createOutputContextWithPath(path):
	global verbose
	if verbose:
		print "Setting %s as the destination." % (path)
	return CGPDFContextCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, path, len(path), False), None, None)

def contextDone(context):
	if context:
		CGPDFContextClose(context)
		del context


class PDFSource:

	def createSource(cls, filename):
		if filename == None:
			instance = PDFSourceStdin()
		else:
			instance = PDFSourceFile(filename)
		return instance
	createSource = classmethod(createSource)

	def __init__(self, filename):
		self.context = None

	def getPDFDoc(self):
		return None

	def getNumberOfPages(self):
		num = 0
		doc = self.getPDFDoc()
		if doc:
			num = CGPDFDocumentGetNumberOfPages(doc)
		return num

	def copyPages(self, writeContext, pageList) :
		"""Copy pages to the supplied PDF context."""
		global verbose

		# If the context isn't supplied, then create a temp file on our own
		if writeContext == None:
			out = PDFContextToStdout()
			writeContext = out.getContext()

		# If the page list is just an integer then create a 1 element list
		# This lets the --slice option be used in a form that returns a single
		# element. For example, [-1] to get the last page.
		if isinstance(pageList, int) :
			pageList = (pageList, )

		self.pdfDoc = self.getPDFDoc()
		self.context = writeContext

		# Copy each requested page to the context creating the new PDF
		for pageNum in pageList :
			if verbose:
				print "Copying page %d of %s" % (pageNum, self.pdfDoc)

			page = CGPDFDocumentGetPage(self.pdfDoc, pageNum)
			if page:
				mediaBox = CGPDFPageGetBoxRect(page, kCGPDFMediaBox)
				if CGRectIsEmpty(mediaBox):
					mediaBox = None

				CGContextBeginPage(writeContext, mediaBox)
				CGContextDrawPDFPage(writeContext, page)
				CGContextEndPage(writeContext)

	def __del__(self):
		contextDone(self.context)


class PDFSourceFile(PDFSource):
	def __init__(self, filename):
		PDFSource.__init__(self, filename)
		self.filename = filename
		self.pdfDoc = None

	def getPDFDoc(self):
		if self.pdfDoc == None:
			self.pdfDoc = createPDFDocumentWithPath(self.filename)
		return self.pdfDoc


class PDFSourceStdin(PDFSource):
	def __init__(self):
		PDFSource.__init__(self)

	def getPDFDoc(self):
		if self.pdfDoc == None:  
			temp = tempfile.NamedTemporaryFile()
			shutil.copyfileobj(sys.stdin, temp, 4096 * 256)
			temp.seek(0)
			self.pdfDoc = createPDFDocumentWithPath(temp.name)
			temp.close()
		return self.pdfDoc


class PDFContextToStdout:
	def __init__(self):
		self.temp = tempfile.NamedTemporaryFile()
		self.context = createOutputContextWithPath(self.temp.name)

	def getContext(self):
		return self.context

	def __del__(self):
		contextDone(self.context)
		self.temp.seek(0)
		shutil.copyfileobj(self.temp, sys.stdout, 4096 * 256)


def main(argv):
	global verbose

	readPDF = None
	readPDFPageCount = 0

	writeFileName = ""
	writeContext = None

	# Parse the command line options
	try:
		options, args = getopt.getopt(argv, "i:o:des:pv", ["input=", "output=", "odd", "even", "slice=", "preview", "verbose"])

	except getopt.GetoptError:
		usage()
		sys.exit(2)

	for option, arg in options:

		if option in ("-i", "--input") :
			if arg:
				del readPDF
				readPDF = PDFSource.createSource(arg)
				readPDFPageCount = readPDF.getNumberOfPages()

		elif option in ("-o", "--output") :
			writeFilename = arg
			if writeFilename:
				contextDone(writeContext)
				writeContext = createOutputContextWithPath(writeFilename)

		elif option in ("-e", "--even") :
			if verbose :
				print "Copy ven pages"

			readPDF.copyPages(writeContext, xrange(2, readPDFPageCount + 1, 2))

		elif option in ("-d", "--odd") :
			if verbose :
				print "Copy odd pages"

			readPDF.copyPages(writeContext, xrange(1, readPDFPageCount + 1, 2))

		elif option in ("-s", "--slice") :

			if verbose :
				print "Slice pages %s" % (arg)

			pageList = range(1, readPDFPageCount + 1)
			scope = {}
			scope['list'] = pageList
			readPDF.copyPages(writeContext, eval('list%s' % (arg), scope))

		elif option in ("-p", "--preview") :
			del readPDF		# In order to flush the PDF file, need to close the context via closing the source PDF
			if verbose :
				print "Open %s in preview" % (writeFilename)
			os.system('open -a "Preview" %s' % (writeFilename))

		elif option in ("-v", "--verbose") :
			print "Verbose mode enabled."
			verbose = True

		else :
			print "Unknown argument: %s" % (option)

	return None


def usage():
	print "Usage: extract [--input <file>] [--output <file>] [--odd] [--even] [--slice <slice>] [--preview] [--verbose]"



if __name__ == "__main__":
	print sys.argv
	main(sys.argv[1:])

    
#! /usr/bin/python
#
# graphpaper
#	Create graphpaper.
#
# graphpaper [--points <points>] [ --gridColor_red <[0-1]>] [--_gridColor_green <0-1>] [--gridColor_blue <0-1>] [--gridColor_alpha <0-1>] [--over 0|1]"
#
#
import sys
import os
import getopt
import tempfile
import shutil
from Quartz.CoreGraphics import *


def drawGrid(pdf, mediaBox, points, red, green, blue, alpha) :

	CGContextSetLineWidth(pdf, 0.1)
	CGContextSetRGBStrokeColor(pdf, red, green, blue, alpha)

	x = mediaBox.origin.x
	while x <= mediaBox.size.width:
		CGContextMoveToPoint(pdf, x, mediaBox.origin.y)
		CGContextAddLineToPoint(pdf, x, mediaBox.origin.y + mediaBox.size.height)
		CGContextStrokePath(pdf)
		x += points

	y = mediaBox.origin.y
	while y < mediaBox.size.height:
		CGContextMoveToPoint(pdf, mediaBox.origin.x, y)
		CGContextAddLineToPoint(pdf, mediaBox.origin.x + mediaBox.size.width, y)
		CGContextStrokePath(pdf)
		y += points	

def usage():
	print "Usage: graphpaper [--points <points>] [ --gridColor_red <[0-1]>] [--_gridColor_green <0-1>] [--gridColor_blue <0-1>] [--gridColor_alpha <0-1>] [--over 0|1] --output <file>"

def main(argv):

	# Parse the command line options
	try:
		options, args = getopt.getopt(argv, "p:r:g:b:a:v:o:i:", ["points=", "gridColor_red=", "gridColor_green=", "gridColor_blue=", "gridColor_alpha=", "over=", "output=", "input="])
	except getopt.GetoptError:
		usage()
		sys.exit(2)
    
	# The distance, in points, between each grid line.
	# Default is a 1/2"
	points = 72 / 4
	
	# The color of the grid lines
	# Default is black
	red = 0
	green = 0
	blue = 0
	alpha = 1
	
	over = 1
	
	writeContext = None
	
	for option, arg in options:
		
		if option in ("-p", "--points") :
			points = float(arg)
			
		elif option in ("-r", "--gridColor_red") :
			red = float(arg)
			if red < 0:
				red = 0
			elif red > 1:
				red = 1
			
		elif option in ("-g", "--gridColor_green") :
			green = float(arg)
			if green < 0:
				green = 0
			elif green > 1:
				green = 1
			
		elif option in ("-b", "--gridColor_blue") :
			blue = float(arg)
			if blue < 0:
				blue = 0
			elif blue > 1:
				blue = 1
			
		elif option in ("-a", "--gridColor_alpha") :
			alpha = float(arg)
			if alpha < 0:
				alpha = 0
			elif alpha > 1:
				alpha = 1
		
		elif option in ("-v", "--over") :
			over = int(arg) != 0
			
		elif option in ("-o", "--output") :
			writeContext = CGPDFContextCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, arg, len(arg), False), None, None)
			
		elif option in ("-i", "--input") :
			readPDF = CGPDFDocumentCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, arg, len(arg), False))
		
		else :
			print "Unknown option %s" % (option)
	
	if writeContext != None and readPDF != None:
		numPages = CGPDFDocumentGetNumberOfPages(readPDF)
		for pageNum in xrange(1, numPages + 1):
		
			page = CGPDFDocumentGetPage(readPDF, pageNum)
			if page:
				mediaBox = CGPDFPageGetBoxRect(page, kCGPDFMediaBox)
				if CGRectIsEmpty(mediaBox):
					mediaBox = None
					
				CGContextBeginPage(writeContext, mediaBox)
			
				if (not over) :
					drawGrid(writeContext, mediaBox, points, red, green, blue, alpha)

				CGContextDrawPDFPage(writeContext, page)
	
				if (over) :
					drawGrid(writeContext, mediaBox, points, red, green, blue, alpha)
				
				CGContextEndPage(writeContext)
				
		CGPDFContextClose(writeContext)
		del writeContext
			
	else:
		print "A valid input file output file must be supplied."
		usage()
		sys.exit(1)
		
if __name__ == "__main__":
	main(sys.argv[1:])
    
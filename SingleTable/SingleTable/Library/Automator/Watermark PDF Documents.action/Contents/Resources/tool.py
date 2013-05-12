#!/usr/bin/python
# Watermark each page in a PDF document

import sys #, os
import getopt
import math
from Quartz.CoreGraphics import *
from Quartz.ImageIO import *

def drawWatermark(ctx, image, xOffset, yOffset, angle, scale, opacity):
	if image:
		imageWidth = CGImageGetWidth(image)
		imageHeight = CGImageGetHeight(image)
		imageBox = CGRectMake(0, 0, imageWidth, imageHeight)
		
		CGContextSaveGState(ctx)
		CGContextSetAlpha(ctx, opacity)
		CGContextTranslateCTM(ctx, xOffset, yOffset)
		CGContextScaleCTM(ctx, scale, scale)
		CGContextTranslateCTM(ctx, imageWidth / 2, imageHeight / 2)
		CGContextRotateCTM(ctx, angle * math.pi / 180)
		CGContextTranslateCTM(ctx, -imageWidth / 2, -imageHeight / 2)
		CGContextDrawImage(ctx, imageBox, image)
		CGContextRestoreGState(ctx)

def createImage(imagePath):
	image = None
	provider = CGDataProviderCreateWithFilename(imagePath)
	if provider:
		imageSrc = CGImageSourceCreateWithDataProvider(provider, None)
		if imageSrc:
			image = CGImageSourceCreateImageAtIndex(imageSrc, 0, None)
	if not image:
		print "Cannot import the image from file %s" % imagePath
	return image

def watermark(inputFile, watermarkFiles, outputFile, under, xOffset, yOffset, angle, scale, opacity, verbose):
	
	images = map(createImage, watermarkFiles)
	
	ctx = CGPDFContextCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, outputFile, len(outputFile), False), None, None)
	if ctx:
		pdf = CGPDFDocumentCreateWithURL(CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, inputFile, len(inputFile), False))
		if pdf:
	
			for i in range(1, CGPDFDocumentGetNumberOfPages(pdf) + 1):
				image = images[i % len(images) - 1]
				page = CGPDFDocumentGetPage(pdf, i)
				if page:
					mediaBox = CGPDFPageGetBoxRect(page, kCGPDFMediaBox)
					if CGRectIsEmpty(mediaBox):
						mediaBox = None
		
					CGContextBeginPage(ctx, mediaBox)
					if under:
						drawWatermark(ctx, image, xOffset, yOffset, angle, scale, opacity)
					CGContextDrawPDFPage(ctx, page)
					if not under:
						drawWatermark(ctx, image, xOffset, yOffset, angle, scale, opacity)
					CGContextEndPage(ctx)
					
			del pdf
		CGPDFContextClose(ctx)
		del ctx

def main(argv):

	verbose = False
	readFilename = None
	writeFilename = None
	under = False
	xOffset = 0
	yOffset = 0
	angle = 0
	opacity = 1.0
	
	# Parse the command line options
	try:
		options, args = getopt.getopt(argv, "vutx:y:a:p:s:i:o:", ["verbose", "under", "over", "xOffset=", "yOffset=", "angle=", "opacity=", "scale=", "input=", "output=", ])
	except getopt.GetoptError:
		usage()
		sys.exit(2)
		
	for option, arg in options:
	
		print option, arg
		
		if option in ("-i", "--input") :
			if verbose:
				print "Reading pages from %s." % (arg)
			readFilename = arg

		elif option in ("-o", "--output") :
			if verbose:
				print "Setting %s as the output." % (arg)
			writeFilename = arg

		elif option in ("-v", "--verbose") :
			print "Verbose mode enabled."
			verbose = True

		elif option in ("-u", "--under"):
			print "watermark under PDF"
			under = True

		elif option in ("t", "--over"):
			print "watermark over PDF"
			under = False
					
		elif option in ("-x", "--xOffset"):
			xOffset = float(arg)
			
		elif option in ("-y", "--yOffset"):
			yOffset = float(arg)
			
		elif option in ("-a", "--angle"):
			angle = -float(arg)

		elif option in ("-s", "--scale"):
			scale = float(arg)
			
		elif option in ("-p", "--opacity"):
			opacity = float(arg)
			
		else:
			print "Unknown argument: %s" % (option)

	if (len(args) > 0):
		watermark(readFilename, args, writeFilename, under, xOffset, yOffset, angle, scale, opacity, verbose);
	else:
		shutil.copyfile(readFilename, writeFilename); 
		
def usage():
	print "Usage: watermark --input <file> --output <file> <watermark files>..."
	
if __name__ == "__main__":
	print sys.argv
	main(sys.argv[1:])
	

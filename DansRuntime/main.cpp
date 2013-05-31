/*
 *  main.c
 *  DansRuntime
 *
 *  Created by Uli Kusterer on 24.03.07.
 *  Copyright 2007 M. Uli Kusterer. All rights reserved.
 *
 */

// -----------------------------------------------------------------------------
//	Headers:
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

//typedef size_t off_t;


// -----------------------------------------------------------------------------
//	Constants:
// -----------------------------------------------------------------------------

// Set this to 1 to use the incremental collector, otherwise we scan all objects
//	each time through Collect().
#define	USE_INCREMENTAL_COLLECTOR	0

// Flags for instance variable entries in runtime type information:
#define DRIVARFLAG_OBJECT		(1 << 0)	// This instance variable is a reference to an object. Used by the garbage collector.

// Flags for objects:
#define DROBJFLAG_MARKER_BUSY	(1 << 0)	// Set on an object to make assignment know the GC's marker is working on it, so we can restart the GC.

// The following strings can be compared by pointer-comparison
//	if you always use these globals to refer to them. You could just as well
//	use ints or whatever, but this has the advantage that you can actually
//	printf() these strings during debugging etc.
typedef const char*		DRSelector;

DRSelector		DRSelector_Create = "Create";
DRSelector		DRSelector_Prepare = "Prepare";
DRSelector		DRSelector_Delete = "Delete";
DRSelector		DRSelector_name = "name";
DRSelector		DRSelector_Object = "Object";
DRSelector		DRSelector_ObjectSubclass = "ObjectSubclass";

// If you want to add your own selectors, define a global containing a pointer to that string and use only that.


// -----------------------------------------------------------------------------
//	Data structures:
// -----------------------------------------------------------------------------

// ----- The following data structures are kept once per class: -----

// An entry for one instance method of a class:
typedef struct DRVTableEntry
{
	DRSelector		mMethodSelector;		// Name of this method (introspection, unique lookup).
	void*			mImpProc;				// Ptr to function that actually implements this method.
} DRVTableEntry;

// An entry for one instance variable of a class:
//	Used for runtime introspection. Also used by our garbage collector to find
//	references to other objects.
typedef struct DRIVarTableEntry
{
	DRSelector		mSelector;				// Name of this instance variable (introspection, unique lookup).
	size_t			mOffset;				// Offset of this instance variable relative to class's iVar offset.
	unsigned int	mFlags;					// Flags with information about this instance variable.
} DRIVarTableEntry;

// This describes a class, its methods, super class, instance variables etc:
typedef struct DRClassInfo
{
	struct DRClassInfo*		mSuperClass;		// NULL for root classes.
	DRSelector				mClassName;			// Name of this class (debugging, introspection).
	size_t					mIvarSize;			// Size of this class's instance vars.
	off_t					mIvarOffset;		// Offset to ivars of this class.
	DRVTableEntry*			mMethods;			// Methods this class implements. Selector/IMP of NULL means end of method list.
	DRIVarTableEntry*		mIVars;				// Instance variables in this class. Selector of NULL means end of ivar list.
} DRClassInfo;


// ----- The following data structure is the basis for every object: -----
// Try to keep the number of instance variables in DRObject itself low. They
// are needed for each object, and thus add a lot of overhead to each object.

// This describes one object:
//	Except for mClass, all of these instance vars wouldn't be needed if we didn't
//	do garbage collection.
typedef struct DRObject
{
	DRClassInfo*		mClass;			// This object's class (i.e. its "ISA").
	unsigned int		mFlags;			// Flags of this object. E.g. the GC uses this to mark our object as busy.
	unsigned int		mLastCycleSeen;	// Last time our GC's marker saw this object.
	struct DRObject*	mNext;			// Next object in linked list of objects.
	struct DRObject*	mPrev;			// Previous object in linked list of objects.
	const char*			mDebugName;		// Name to show for this object as a debugging aid.

	// ivars go here.
} DRObject;


// -----------------------------------------------------------------------------
//	Globals:
// -----------------------------------------------------------------------------

// Doubly linked list of all objects used by garbage collector:
DRObject*				gFirstObject = NULL;
DRObject*				gLastObject = NULL;

	/* Why a linked list? Well, a linked list can be changed while someone is
		iterating through it without too many problems, because all element
		pointers except one that is deleted stay valid. It's also fast to
		add and remove elements (constant-time) and thus faster as re-allocating
		a huge array each time. A doubly-linked list makes this even faster, as
		we don't have to walk the list to find the previous object to update its
		"next" pointer. */

// Incremental garbage collector's state information:
DRObject*				gCurrCollectingPosition = NULL;	// Position inside linked list where garbage collector is currently busy.
unsigned int			gCurrGCMarkCycle = 0;			// Cycle counter (i.e. "generation year") for the marker part of our incremental garbage collector. This is also used by the mark/sweep-all-at-once collector.
unsigned int			gCurrGCSweepCycle = 0;			// Cycle counter used for the "sweep" part of our incremental garbage collector. *not* used by the mark/sweep-all-at-once collector.

DRObject*				gReachableRoot = NULL;			// The object that serves as the root of our tree of reachable objects.
														//	For a real-world app, you would probably want this to be a list of
														//	objects.


// -----------------------------------------------------------------------------
//	Forwards:
// -----------------------------------------------------------------------------

void*		FindMethodImp( DRSelector methodName, DRClassInfo* info );


// -----------------------------------------------------------------------------
//	Instance methods for classes created using our runtime:
// -----------------------------------------------------------------------------

// Class "Object":
void	DRClassObject_Create( DRObject* self )
{
	printf( "Created Object %s.\n", self->mDebugName );
}


void	DRClassObject_Delete( DRObject* self )
{
	printf( "Deleted Object %s.\n", self->mDebugName );
}


// Class "ObjectSubclass":
void	DRClassObjectSubclass_Create( DRObject* self )
{
	void		(*theMethod)( DRObject* self ) = NULL;
	
	printf( "Calling superclass constructor for %s:\n\t", self->mDebugName );
	theMethod = (void (*) (DRObject*)) FindMethodImp( DRSelector_Create, self->mClass->mSuperClass );	// Find method in our superclass.
	if( theMethod )
		theMethod( self );	// Run it, if superclass implements it.
	else
		printf( "Superclass %s doesn't implement \"Create\".\n", self->mClass->mSuperClass->mClassName );

	printf( "Created ObjectSubclass %s.\n", self->mDebugName );
}


// -----------------------------------------------------------------------------
//	VTable, IVar table and class info for class "Object":
// -----------------------------------------------------------------------------

// Here we look up the actual functions that a particular class overrides or
//	makes available to be overridden:
DRVTableEntry	gClassObjectVTable[] =
{
	{ DRSelector_Create, (void*) DRClassObject_Create },
	{ DRSelector_Delete, (void*) DRClassObject_Delete },
	{ NULL, NULL }
};

	/* Instead of searching for the selector, you could also just save the
		index of the respective entry, and make a copy of the superclass's
		list of selectors and then append your new methods and replace
		the ones you override. However, such an approach would also suffer
		from the fragile base class problem, and would mean that your program
		could only send messages to objects that actually are of the class you
		expect. OTOH this approach lets any object that understands a particular
		message handle it, even if it's another class than the one you expected. */

// This is information about any DRObject* variables that objects of this class
//	contain, so the garbage collector can determine reachability of objects:
DRIVarTableEntry	gClassObjectIVarTable[] =
{
	{ NULL, 0, 0 }		// The base class "Object" has no instance vars for now.
};

// The actual class information:
DRClassInfo		gClassObject =
{
	NULL,					// No superclass, this is a root class.
	DRSelector_Object,		// Unique identifier for this class.
	0,						// No instance variables.
	0,						// Placeholder where we'll calculate offset to first iVar.
	gClassObjectVTable,		// Table of virtual functions.
	gClassObjectIVarTable	// Table of object-referencing instance variables.
};


// Same for our subclass "ObjectSubclass":
DRVTableEntry	gClassObjectSubclassVTable[] =
{
	{ DRSelector_Create, (void*) DRClassObjectSubclass_Create },	// We override "create".
	{ NULL, NULL }
};

DRIVarTableEntry	gClassObjectSubclassIVarTable[] =
{
	{ DRSelector_name, 0, DRIVARFLAG_OBJECT },	// DRObject*  name;
	{ NULL, 0, 0 }
};

// A subclass of our "Object" class:
DRClassInfo		gClassObjectSubclass =
{
	&gClassObject,					// Superclass "Object".
	DRSelector_ObjectSubclass,		// Unique identifier for this class.
	sizeof(DRObject*),				// Size of instance variable data (one object reference).
	0,								// Placeholder for iVar offset.
	gClassObjectSubclassVTable,		// Table of virtual functions.
	gClassObjectSubclassIVarTable	// Table of instance vars.
};


struct DRIncrementalMarkerStackEntry
{
	DRObject*		mObject;	// Object currently being scanned.
	unsigned int	mIndex;		// Index of instance variable being scanned in this object.
};


// -----------------------------------------------------------------------------
//	Marker Stack:
//		Stack push/peek/pop functions for the stack that our marker phase uses
//		to keep track of what it last marked.
// -----------------------------------------------------------------------------

#define	MARKER_STACK_BLOCK_SIZE		16	// We allocate more space for the stack in blocks of this many entries. That way, we don't reallocate for every little "push".

DRIncrementalMarkerStackEntry*		gIncrementalMarkerStack = NULL;		// Storage for stack.
unsigned int						gIncrementalMarkerStackSize = 0;	// Actual size of memory in gIncrementalMarkerStack (in entries, not bytes).
unsigned int						gIncrementalMarkerStackBack = 0;	// Number of entries currently used in marker stack.


DRIncrementalMarkerStackEntry*		PushOntoMarkerStack( DRObject* obj, int ind )	// Pointer returned is invalid as soon as you push/pop the next time.
{
	if( !gIncrementalMarkerStack )
	{
		gIncrementalMarkerStackSize = MARKER_STACK_BLOCK_SIZE;
		gIncrementalMarkerStack = (DRIncrementalMarkerStackEntry*) malloc( gIncrementalMarkerStackSize * sizeof(DRIncrementalMarkerStackEntry) );
	}
	else if( gIncrementalMarkerStackSize < gIncrementalMarkerStackBack )
	{
		gIncrementalMarkerStackSize += MARKER_STACK_BLOCK_SIZE;
		gIncrementalMarkerStack = (DRIncrementalMarkerStackEntry*) realloc( gIncrementalMarkerStack, gIncrementalMarkerStackSize * sizeof(DRIncrementalMarkerStackEntry) );
	}
	
	gIncrementalMarkerStackBack++;
	
	DRIncrementalMarkerStackEntry*	backEntry = gIncrementalMarkerStack +(gIncrementalMarkerStackBack -1);
	backEntry->mObject = obj;
	backEntry->mIndex = ind;
	
	obj->mFlags |= DROBJFLAG_MARKER_BUSY;
	
	return backEntry;
}


DRIncrementalMarkerStackEntry*	BackOfMarkerStack()	// Pointer returned is invalid as soon as you push/pop the next time.
{
	if( !gIncrementalMarkerStack || gIncrementalMarkerStackBack == 0 )
		return NULL;
	
	DRIncrementalMarkerStackEntry*	backEntry = gIncrementalMarkerStack +(gIncrementalMarkerStackBack -1);
	return backEntry;
}


DRIncrementalMarkerStackEntry*	PopFromMarkerStack()
{
	if( !gIncrementalMarkerStack || gIncrementalMarkerStackBack == 0 )
		return NULL;
	
	DRIncrementalMarkerStackEntry*	backEntry = gIncrementalMarkerStack +(gIncrementalMarkerStackBack -1);
	backEntry->mObject->mFlags &= ~DROBJFLAG_MARKER_BUSY;
	
	gIncrementalMarkerStackBack--;	// We don't make the actual memory block smaller, we'll probably need the memory again soon.
									// If you're worried about memory usage, you can change that, but then this can't return its last entry as easily.
	
	return backEntry;
}


// -----------------------------------------------------------------------------
//	RewindMarkerStackToObject ()
//		Call this to reset the "mark" phase of our incremental garbage collector
//		when you move an object in the hierarchy. This does nothing if the
//		garbage collector isn't busy with an object, so it's a pretty cheap
//		call.
//
//		During the "sweep" phase that deletes the objects, our garbage collector
//		walks the internal linked list of objects, so there is no way it can
//		change between calls (because the sweep phase is the only one that can
//		delete an object).
//
//		During the "mark" phase, however, we walk the actual object hierarchy
//		as users of this runtime see it. Since the user of this runtime may at
//		any time between calls to MarkNextReachableObject() take the object
//		being marked out of its IVar we reached it by and put another object
//		in the IVar that we may not have marked yet, we need to re-scan the
//		object. We do this by rewinding the stack MarkNextReachableObject()
//		uses to keep track of where in the object hierarchy it currently is.
// -----------------------------------------------------------------------------

void	RewindMarkerStackToObject_Costly( DRObject* obj )
{
	DRIncrementalMarkerStackEntry* currEntry = NULL;
	do
	{
		currEntry = PopFromMarkerStack();
	}
	while( currEntry && currEntry->mObject != obj );
	currEntry = BackOfMarkerStack();
	if( currEntry && currEntry->mIndex > 0 )
		currEntry->mIndex--;	// Re-scan IVar we reached the other object by.
}

inline void	RewindMarkerStackToObject( DRObject* obj )	// Avoid function call overhead for 90% of the cases using this.
{
	if( ((obj)->mFlags & DROBJFLAG_MARKER_BUSY) == DROBJFLAG_MARKER_BUSY )
		RewindMarkerStackToObject_Costly((obj));
}



// -----------------------------------------------------------------------------
//	CalculateIvarOffset ()
//		This is used to update the mIVarOffset field of each class, so our code
//		can quickly access instance variables. We could hard-code the offset
//		into the table, but by culating it at runtime, we can avoid the
//		"fragile base class"-problem.
//
//		Fragile Base Class:
//			The instance vars of an object are stored in sequence, with the
//			superclass's instance vars at the top and the ones the class adds
//			below them. We calculate the offsets of each instance variable and
//			use it to access its data. If the superclass is loaded from a
//			dynamic library (e.g. because it is a system library), and that
//			library gets updated and a new instance variable is added to the
//			superclass, all the offsets hard-coded into the subclasses would be
//			wrong.
//			
//		What we do here is we have each class note down its size, and then
//		we can calculate the offset of each class's ivars at runtime, based
//		on the sizes of the classes actually loaded, and not based on the
//		sizes as they were when we compiled. This makes accessing instance
//		variables a tad more complicated, as we need to look up the offset
//		of each iVar at runtime, but it works.
//			
//		Of course we could pre-calculate the offset of each instance variable,
//		but I chose to assemble it from a general "ivar offset" for each class,
//		plus offsets relative to that for each instance variable. By doing this,
//		we could theoretically resize a class at runtime or do other fancy
//		things.
// -----------------------------------------------------------------------------

void	CalculateIvarOffset( DRClassInfo* info )
{
	if( info->mIvarOffset != 0 )
		return;
	if( info->mSuperClass == NULL )
		info->mIvarOffset = sizeof(DRObject);
	else
	{
		if( info->mSuperClass->mIvarOffset == 0 )
			CalculateIvarOffset( info->mSuperClass );
		info->mIvarOffset = info->mSuperClass->mIvarOffset +info->mSuperClass->mIvarSize;
	}
}


// -----------------------------------------------------------------------------
//	FindMethodImp ()
//		Each method has a name or "selector" that identifies it relative to
//		its object, as well as an implementation function ("IMP" for short)
//		that actually runs the code for it.
//
//		When you want to call a method on an object, look up the IMP to call
//		using this function. This will look for the method in the specified
//		class and all superclasses, returning NULL if nobody implements the
//		selector.
// -----------------------------------------------------------------------------

void*		FindMethodImp( DRSelector methodName, DRClassInfo* info )
{
	DRClassInfo*	currInfo = info;
	while( currInfo )
	{
		int				x = 0;
		DRVTableEntry*	currMethod = currInfo->mMethods +x;
		while( currMethod->mMethodSelector != NULL )
		{
			if( currMethod->mMethodSelector == methodName )
				return currMethod->mImpProc;
			
			currMethod = currInfo->mMethods +(++x); 
		}
		
		currInfo = currInfo->mSuperClass;
	}
	
	return NULL;
}


// -----------------------------------------------------------------------------
//	AllocObjectOfClass ()
//		Allocates the memory for an object with a particular class and
//		initializes its Class pointer and garbage collector info ("meta-ivars").
// -----------------------------------------------------------------------------

DRObject*	AllocObjectOfClass( DRClassInfo* info, const char* debugName )
{
	CalculateIvarOffset( info );
    DRObject*	theObject = (DRObject*) calloc( info->mIvarOffset +info->mIvarSize, 1 );	// Zeroes out our new memory.
	
	// Initialize meta-ivars before anyone else gets to see this object:
	theObject->mClass = info;
	theObject->mLastCycleSeen = gCurrGCMarkCycle;	// Start out as reachable so we don't get collected by accident.
	theObject->mDebugName = debugName;

	// Insert us at start of linked list:
	if( !gFirstObject )
	{
		gFirstObject = theObject;
		gLastObject = theObject;
	}
	else
	{
		gFirstObject->mPrev = theObject;
		theObject->mNext = gFirstObject;
		gFirstObject = theObject;
	}
	
	return theObject;
}


// -----------------------------------------------------------------------------
//	CreateObjectOfClass ()
//		Creates a new object (allocating its memory using AllocObjectOfClass)
//		and then calls its constructor and its preparation method.
//
//		The preparation method is for initialization stuff that you can't call
//		until you're sure your superclass has been initialized.
// -----------------------------------------------------------------------------

DRObject*	CreateObjectOfClass( DRClassInfo* info, const char* debugName )
{
	void		(*theMethod)( DRObject* self ) = NULL;
	DRObject*	theObject = NULL;
	theObject = AllocObjectOfClass( info, debugName );
	
	// Send "Create" message:
	theMethod = (void (*) (DRObject*)) FindMethodImp( DRSelector_Create, theObject->mClass );
	if( theMethod )
		theMethod( theObject );
	else
		printf( "Class %s or superclasses don't implement \"Create\".\n", info->mClassName );
	
	// Send "Prepare" message:
	theMethod = (void (*) (DRObject*)) FindMethodImp( DRSelector_Prepare, theObject->mClass );
	if( theMethod )
		theMethod( theObject );
	else
		printf( "Class %s or superclasses don't implement \"Prepare\".\n", info->mClassName );
	
	return theObject;
}


// -----------------------------------------------------------------------------
//	ClassDescendsFromClass ()
//		Introspection method that tells you whether one class is a subclass
//		of another. Since you can ask an object for its class, this can also be
//		used on objects.
// -----------------------------------------------------------------------------

bool	ClassDescendsFromClass( DRClassInfo* currClass, DRClassInfo* superClass )
{
	while( currClass )
	{
		if( currClass == superClass )
			return true;
		currClass = currClass->mSuperClass;
	}
	
	return false;
}


// -----------------------------------------------------------------------------
//	DeleteObject ()
//		Function used internally by the garbage collector to dispose of an
//		object. Unless you decide not to use the garbage collector *at all*,
//		you should never call this method.
// -----------------------------------------------------------------------------

void	DeleteObject( DRObject* theObject )
{
	void		(*theMethod)( DRObject* self ) = NULL;
	
	// Call "Delete" method to let object know it's going:
	theMethod = (void (*) (DRObject*)) FindMethodImp( DRSelector_Delete, theObject->mClass );
	if( theMethod )
		theMethod( theObject );
	else
		printf( "Class %s or superclasses don't implement \"Delete\".\n", theObject->mClass->mClassName );
	
	// Take us out of our linked list of objects:
	if( theObject->mPrev )
		theObject->mPrev->mNext = theObject->mNext;
	else
		gFirstObject = theObject->mNext;
	if( theObject->mNext )
		theObject->mNext->mPrev = theObject->mPrev;
	else
		gLastObject = theObject->mPrev;
	
	free( theObject );
}


// -----------------------------------------------------------------------------
//	MarkAllObjectsUnreachable ()
//		Goes through our linked list of objects and marks each one as not
//		reachable, i.e. eligible for being garbage-collected. Once you have
//		done that, you can call MarkReachableObjects() to mark all objects that
//		are still in use, and then you can ask the garbage collector to delete
//		the unused objects by calling DeleteAllUnreachableObjects().
//
//		This is the classic behaviour of a mark/sweep garbage collector.
//		A disadvantage of this is that your app may take a lunch break while
//		it does all this collecting. And you don't want it to be interrupted
//		in the mean time.
//		See DeleteNextUnreachableObject() if you're interested in doing at
//		least some of this work in the background.
// -----------------------------------------------------------------------------

void	MarkAllObjectsUnreachable()
{
	DRObject*		currObj = gFirstObject;
	
	while( currObj )
	{
		currObj->mLastCycleSeen = gCurrGCMarkCycle -3;	// See DeleteNextUnreachableObject() for why we need -3 here.
		currObj = currObj->mNext;
	}
}


// -----------------------------------------------------------------------------
//	MarkAllObjectsReachable ()
//		Goes through our linked list of objects and marks each one as reachable,
//		thus postponing collection of some objects.
//
//		This is used in cases where our gCurrGCMarkCycle counter gets so large
//		that it gets cut off and ends up being zero again. Since now, all old
//		objects would look incredibly young and would never get collected, we
//		use this to re-date them all to the new cycle number (probably 0). That
//		way they will get collected in a few cycles.
// -----------------------------------------------------------------------------

void	MarkAllObjectsReachable()
{
	DRObject*		currObj = gFirstObject;
	
	while( currObj )
	{
		currObj->mLastCycleSeen = gCurrGCMarkCycle;
		currObj = currObj->mNext;
	}
}


// -----------------------------------------------------------------------------
//	DeleteAllUnreachableObjects ()
//		Loops over our list of objects and deletes all objects that have been
//		determined as unreachable during the last pass of marking using
//		MarkReachableObjects().
//
//		This is used for the sweep phase of our mark/sweep garbage collector.
//		After all objects have been marked as unreachable, we mark those that
//		are actually reachable as such, and what's left over will be the objects
//		that are actually uncreachable, which this function will then delete.
//		
//		We don't use this for our incremental garbage collector.
// -----------------------------------------------------------------------------

void	DeleteAllUnreachableObjects()
{
	DRObject*				currObj = gFirstObject;
	
	while( currObj )
	{
		DRObject*	checkedObject = currObj;
		currObj = currObj->mNext;	// Already go to next object so we have a next pointer even if we delete this object now.
		if( checkedObject->mLastCycleSeen < (gCurrGCMarkCycle -1) )
			DeleteObject( checkedObject );
	}
}


// -----------------------------------------------------------------------------
//	MarkReachableObjects ()
//		Marks all objects hanging off of the object in gReachableRoot as being
//		reachable. This is part of our mark/sweep garbage collector, where we
//		first mark all objects as unreachable, then call this to mark our tree
//		of objects as reachable (you should add all objects to this tree, even
//		ones that currently only have a local variable on the stack pointing
//		at them. I.e. ideally, your programming language's call stack would
//		also be an object in this tree.
//		
//		We don't use this for our incremental garbage collector.
// -----------------------------------------------------------------------------

void	MarkOneObjectReachable( DRObject* obj );

inline void	MarkReachableObjects()
{
	MarkOneObjectReachable(gReachableRoot);
	gCurrGCMarkCycle++;
}


// -----------------------------------------------------------------------------
//	MarkOneObjectReachable ()
//		Mark one object and all the objects it refers to as reachable so they
//		aren't garbage-collected.
//
//		Used by our mark-sweep garbage collector to mark objects as reachable.
//		We don't use this for our incremental garbage collector.
// -----------------------------------------------------------------------------

void	MarkOneObjectReachable( DRObject* obj )
{
	if( !obj || obj->mLastCycleSeen == gCurrGCMarkCycle )	// Not an object or we already did it?
		return;
	
	obj->mLastCycleSeen = gCurrGCMarkCycle;	// Mark this one reachable.
	
	// Mark all sub-objects, too:
	DRIVarTableEntry*	entry = obj->mClass->mIVars;
	while( entry->mSelector != NULL )
	{
		if( (entry->mFlags & DRIVARFLAG_OBJECT) == DRIVARFLAG_OBJECT )
		{
			off_t		theOffset = obj->mClass->mIvarOffset + entry->mOffset;
			DRObject*	theObj = *(DRObject**) (((char*) obj) +theOffset);
			MarkOneObjectReachable( theObj );
		}
		entry++;
	}
}


#define	OUT_OF_OBJECTS		((DRObject*) 1)


// -----------------------------------------------------------------------------
//	GetIndObjectIVarOfObject ()
//		Used by the garbage collector to walk the IVars in each object and
//		thus determine reachability of objects hanging off others.
// -----------------------------------------------------------------------------

DRObject*	GetIndObjectIVarOfObject( unsigned int theIndex, DRObject* container )
{
	DRIVarTableEntry*	entry = container->mClass->mIVars;
	entry += theIndex;
	if( entry->mSelector == NULL )
		return OUT_OF_OBJECTS;
	else if( (entry->mFlags & DRIVARFLAG_OBJECT) == DRIVARFLAG_OBJECT )
	{
		off_t		theOffset = container->mClass->mIvarOffset + entry->mOffset;
		return *(DRObject**) (((char*) container) +theOffset);
	}
	else
		return NULL;
}


// -----------------------------------------------------------------------------
//	SetIndObjectIVarOfObject ()
//		Use this to change object IVars.
// -----------------------------------------------------------------------------

void	SetIndObjectIVarOfObject( unsigned int theIndex, DRObject* theValue, DRObject* container )
{
	DRIVarTableEntry*	entry = container->mClass->mIVars;
	entry += theIndex;
	if( (entry->mFlags & DRIVARFLAG_OBJECT) == DRIVARFLAG_OBJECT )
	{
		off_t		theOffset = container->mClass->mIvarOffset + entry->mOffset;
		*(DRObject**) (((char*) container) +theOffset) = theValue;
	}
}


// -----------------------------------------------------------------------------
//	MarkNextReachableObject ()
//		Main bottleneck of our incremental garbage collector's "mark" phase.
//		This does essentially the same as MarkReachableObjects(), just that
//		instead of using recursion and the call stack to keep track of the
//		current object, how we arrived there and which IVar we're currently
//		checking, this uses a stack in a global variable. That way, each call
//		is roughly equivalent to one loop iteration in MarkOneObjectReachable().
//		
//		However, since there may be other code accessing the objects we're
//		scanning between calls to this function, our marker stack sets the
//		DROBJFLAG_MARKER_BUSY flag on each object that gets pushed on it, and
//		removes the flag when it is removed. That way, anyone who changes a
//		reference to an object the garbage collector is currently working with
//		can use RewindMarkerStackToObject() to rewind this function's stack so
//		it will re-scan the modified object.
// -----------------------------------------------------------------------------

void	MarkNextReachableObject()
{
	DRIncrementalMarkerStackEntry*	currEntry = BackOfMarkerStack();
	if( !currEntry )
	{
		gCurrGCMarkCycle++;

		currEntry = PushOntoMarkerStack( gReachableRoot, 0 );		// Start a new round.
		currEntry->mObject->mLastCycleSeen = gCurrGCMarkCycle;		// Mark root as reachable.
		return;		// Stop once to give IncrementalCollect() a chance to starve us and have the sweeper catch up.
	}
	
	// Find the next object IVar:
	DRObject*	newSubObject = NULL;
	while( newSubObject == NULL )	// NULL == not an object.
		newSubObject = GetIndObjectIVarOfObject( currEntry->mIndex++, currEntry->mObject );
	
	if( newSubObject == OUT_OF_OBJECTS )	// Hit last IVar without any more objects?
		PopFromMarkerStack();				// Finished with this object.
	else if( newSubObject != NULL )
	{
		PushOntoMarkerStack( newSubObject, 0 );				// Schedule for scanning next round.
		newSubObject->mLastCycleSeen = gCurrGCMarkCycle;	// Mark as reachable.
	}
}


// -----------------------------------------------------------------------------
//	DeleteNextUnreachableObject ()
//		This is our incremental garbage collector's "sweep" phase. While
//		DeleteAllUnreachableObjects() runs in one go and may thus cause
//		noticeable pauses when you have a lot of objects, this function will
//		only check one object in our list of reachable objects and then return,
//		maintaining state in a global.
//
//		When objects are marked, they are marked with the most recent garbage
//		collector cycle number, and not just with a reachable/unreachable flag.
//		This way, we can mark objects while the program is running (even during
//		the sweep phase), and we don't need to loop over all objects to mark
//		them unreachable, as older ones have a different cycle number that
//		indicates they're not marked reachable already. Because objects from one
//		cycle ago might not necessarily be unreachable (they could just be ones
//		the marker hasn't processed yet), we only purge objects from two cycles
//		ago, which guarantees the marker has seen them.
// -----------------------------------------------------------------------------

void	DeleteNextUnreachableObject()
{
	if( gCurrCollectingPosition == NULL )
	{
		gCurrCollectingPosition = gFirstObject;
		if( gCurrCollectingPosition == NULL )	// No objects? Nothing to collect.
			return;
		
		// Start a new GC cycle:
		gCurrGCSweepCycle++;
		return;		// Stop once to give IncrementalCollect() a chance to starve us and have the marker catch up.
	}
	
	DRObject*	checkedObject = gCurrCollectingPosition;
	gCurrCollectingPosition = gCurrCollectingPosition->mNext;	// Already go to next object so we have a next pointer even if we delete this object now.
	
	if( checkedObject->mLastCycleSeen < (gCurrGCMarkCycle -1) )	// Haven't seen it for two cycles or more? Delete it.
		DeleteObject( checkedObject );
}


// -----------------------------------------------------------------------------
//	IncrementalCollect ()
//		Our flag that we use for garbage collecting is the number of complete
//		cycles we've absolved so far. Problem is, if one of our two phases
//		takes longer than the other, it will do more cycles. So, what we do is
//		make sure one phase waits for the other once it has finished.
// -----------------------------------------------------------------------------

void	IncrementalCollect( int howOften )
{
	for( int x = 0; x < howOften; x++ )
	{
		// Only do marking if the "sweep" phase is in the same cycle or ahead of us. Otherwise wait until it catches up:
		if( gCurrGCMarkCycle <= gCurrGCSweepCycle					// Mark phase is in lower or same cycle as sweep.
			|| gCurrGCSweepCycle == 0 && gCurrGCMarkCycle > 10 )	// Sweep phase has already overflowed, but mark hasn't -> Mark in lower cycle than sweep.
			MarkNextReachableObject();
		// Only do sweeping if the "mark" phase is in the same cycle or ahead of us. Otherwise wait until it catches up:
		if( gCurrGCSweepCycle <= gCurrGCMarkCycle					// Sweep phase is in lower or same cycle as mark.
			|| gCurrGCMarkCycle == 0 && gCurrGCSweepCycle > 10 )	// Mark phase has already overflowed, but sweep hasn't -> Sweep in lower cycle than mark.
			DeleteNextUnreachableObject();
	}
}


// -----------------------------------------------------------------------------
//	MarkAndSweepCollect ()
//		This loops over the entire list of objects, marks them all as
//		unreachable and then walks our hierarchy of reachable objects and marks
//		them all as reachable again. After that, we can delete all objects that
//		are still marked reachable.
//
//		Since this processes all objects each time, it can be kinda slow with
//		lots of objects, and cause longer pauses in program execution.
// -----------------------------------------------------------------------------

void	MarkAndSweepCollect()
{
	MarkAllObjectsUnreachable();
	MarkReachableObjects();
	DeleteAllUnreachableObjects();
}


// -----------------------------------------------------------------------------
//	Collect ()
//		No matter whether you're using the incremental collector or the
//		one-time-mark-and-sweep-everything variant, this function will do
//		the right thing and do a collection, so call this periodically.
// -----------------------------------------------------------------------------

void	Collect()
{
	#if USE_INCREMENTAL_COLLECTOR
	IncrementalCollect(10);
	#else
	MarkAndSweepCollect();
	#endif
}


// -----------------------------------------------------------------------------
//	main ()
//		Very minimal app that creates some collected objects and collects them.
//		
//		We create one Object "First" which we put in gReachableRoot, and
//		this in turn refers to an object named "SubObject". Hence, those two
//		will not be collected.
//		
//		The third object we create, "Dead", is not referred to by any of the
//		objects in gReachableRoot, so it will get collected at first
//		opportunity.
// -----------------------------------------------------------------------------

int main (int argc, char * const argv[])
{
	DRObject*	firstObject = CreateObjectOfClass( &gClassObjectSubclass, "First" );
	gReachableRoot = firstObject;	// Add first to our tree, so it doesn't get collected.
	DRObject*	deadObject = CreateObjectOfClass( &gClassObject, "Dead" );	// We don't attach this one to anyone else, so it gets collected.
	DRObject*	subObject = CreateObjectOfClass( &gClassObject, "SubObject" );
	SetIndObjectIVarOfObject( 0, subObject, firstObject );	// Attach SubObject to First.
	
	printf("==========\n");
	
	// The following would usually happen in your event loop:
	for( int x = 0; x < 50; x++ )
		Collect();
	
	printf("==========\n");

	// Kill all objects before terminating:
	MarkAllObjectsUnreachable();
	DeleteAllUnreachableObjects();	// Deletes all, since we didn't mark any as reachable.

    return 0;
}

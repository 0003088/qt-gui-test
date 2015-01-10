/***************************************************************************
            split.c  -  Functions for splitting keysets for kdbSet
                             -------------------
    begin                : Fri 21 Mar 2008
    copyright            : (C) 2008 by Markus Raab
    email                : elektra@markus-raab.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the BSD License (revised).                      *
 *                                                                         *
 ***************************************************************************/


#ifdef HAVE_KDBCONFIG_H
#include "kdbconfig.h"
#endif

#if DEBUG && defined(HAVE_STDIO_H)
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <kdberrors.h>
#include <kdbinternal.h>


/**
 * Allocates a new split object.
 *
 * Splits up a keyset into multiple keysets where each
 * of them will passed to the correct kdbSet().
 *
 * Initially the size is APPROXIMATE_NR_OF_BACKENDS.
 *
 * @return a fresh allocated split object
 * @ingroup split
 * @see elektraSplitDel()
**/
Split * elektraSplitNew(void)
{
	Split *ret = elektraCalloc (sizeof (Split));

	ret->size = 0;
	ret->alloc = APPROXIMATE_NR_OF_BACKENDS;

	ret->keysets=elektraCalloc(sizeof(KeySet*) * ret->alloc);
	ret->handles=elektraCalloc(sizeof(KDB *) * ret->alloc);
	ret->parents=elektraCalloc(sizeof(Key *) * ret->alloc);
	ret->syncbits=elektraCalloc(sizeof(int) * ret->alloc);

	return ret;
}


/**
 * Delete a split object.
 *
 * Will free all allocated resources of a split keyset.
 *
 * @param keysets the split object to work with
 * @ingroup split
 */
void elektraSplitDel(Split *keysets)
{
	for (size_t i=0; i<keysets->size; ++i)
	{
		ksDel (keysets->keysets[i]);
		keyDecRef (keysets->parents[i]);
		keyDel (keysets->parents[i]);
	}
	elektraFree (keysets->keysets);
	elektraFree (keysets->handles);
	elektraFree (keysets->parents);
	elektraFree (keysets->syncbits);
	elektraFree (keysets);
}

/**
 * Doubles the size of how many parts of keysets can be appended.
 *
 * @param split the split object to work with
 * @ingroup split
 */
void elektraSplitResize(Split *split)
{
	split->alloc *= 2;

	elektraRealloc((void**) &split->keysets, split->alloc * sizeof(KeySet *));
	elektraRealloc((void**) &split->handles, split->alloc * sizeof(KDB *));
	elektraRealloc((void**) &split->parents, split->alloc * sizeof(Key *));
	elektraRealloc((void**) &split->syncbits, split->alloc * sizeof(int));
}

/**
 * Increases the size of split and appends a new empty keyset.
 *
 * Initializes the element with the given parameters
 * at size-1 to be used.
 *
 * Will automatically resize split if needed.
 *
 * @param split the split object to work with
 * @param backend the backend which should be appended
 * @param parentKey the parentKey which should be appended
 * @param syncbits the initial syncstate which should be appended
 * @ingroup split
 * @retval -1 if no split is found
 * @return the size of split - 1
 */
ssize_t elektraSplitAppend(Split *split, Backend *backend, Key *parentKey, int syncbits)
{
	if (!split)
	{
		/* To make test cases work & valgrind clean */
		keyDel (parentKey);
		return -1;
	}

	++ split->size;
	if (split->size > split->alloc) elektraSplitResize(split);

	split->keysets[split->size-1]=ksNew(0, KS_END);
	split->handles[split->size-1]=backend;
	split->parents[split->size-1]=parentKey;
	split->syncbits[split->size-1]=syncbits;

	return split->size-1;
}

/**
 * Determines if the backend is already inserted or not.
 *
 * @warning If no parent Key is given, the default/root backends won't
 * be searched.
 *
 * @param split the split object to work with
 * @param backend the backend to search for
 * @param parent the key to check for domains in default/root backends.
 * @return pos of backend if it already exist
 * @return -1 if it does not exist
 * @ingroup split
 */
ssize_t elektraSplitSearchBackend(Split *split, Backend *backend, Key *parent)
{
	for (size_t i=0; i<split->size; ++i)
	{
		if (backend == split->handles[i])
		{
			if (split->syncbits[i] & 2)
			{
				switch (keyGetNamespace(parent))
				{
				case KEY_NS_SPEC: if (keyIsSpec(split->parents[i])) return i; break;
				case KEY_NS_DIR: if (keyIsDir(split->parents[i])) return i; break;
				case KEY_NS_USER: if (keyIsUser(split->parents[i])) return i; break;
				case KEY_NS_SYSTEM: if (keyIsSystem(split->parents[i])) return i; break;
				case KEY_NS_PROC: return -1;
				case KEY_NS_EMPTY: return -1;
				case KEY_NS_NONE: return -1;
				case KEY_NS_META: return -1;
				case KEY_NS_CASCADING: return -1;
				}
				continue;
			}
			/* We already have this backend, so leave */
			return i;
		}
	}
	return -1;
}


/**
 * @return 1 if one of the backends in split has all
 *          keys below parentKey
 * @return 0 if parentKey == 0 or there are keys below
 *          or same than parentKey which do not fit
 *          in any of split keysets
 * @param split the split object to work with
 * @param parentKey the key which relation is searched for
 * @ingroup split
 */
int elektraSplitSearchRoot(Split *split, Key *parentKey)
{
	size_t splitSize = split->size;

	if (!parentKey) return 0;

	for (size_t i=0; i<splitSize; ++i)
	{
		if (keyRel (split->parents[i], parentKey) >= 0)
			return 1;
	}

	return 0;
}


/**
 * Walks through kdb->split and adds all backends below parentKey to split.
 *
 * Sets syncbits to 2 if it is a default or root backend (which needs splitting).
 * The information is copied from kdb->split.
 *
 * @pre split needs to be empty, directly after creation with elektraSplitNew().
 *
 * @pre there needs to be a valid defaultBackend
 *      but its ok not to have a trie inside KDB.
 *
 * @pre parentKey must be a valid key! (could be implemented more generally,
 *      but that would require splitting up of keysets of the same backend)
 *
 * @param split will get all backends appended
 * @param kdb the handle to get information about backends
 * @param parentKey the information below which key the backends are from interest
 * @ingroup split
 * @return always 1
 */
int elektraSplitBuildup (Split *split, KDB *kdb, Key *parentKey)
{
	/* For compatibility reasons invalid names are accepted, too.
	 * This solution is faster than checking the name of parentKey
	 * every time in loop.
	 * The parentKey might be null in some unit tests, so also check
	 * for this. */
	if (!parentKey || !strcmp(keyName(parentKey), "") || !strcmp(keyName(parentKey), "/"))
	{
		parentKey = 0;
	}

	/* Returns the backend the key is in or the default backend
	   otherwise */
	Backend * backend = elektraMountGetBackend(kdb, parentKey);

	for (size_t i=0; i < kdb->split->size; ++i)
	{
		if (!parentKey)
		{
			/* Catch all: add all mountpoints */
			elektraSplitAppend (split, kdb->split->handles[i], keyDup(kdb->split->parents[i]), kdb->split->syncbits[i]);
		}
		else if (backend == kdb->split->handles[i] && keyRel(kdb->split->parents[i], parentKey) >= 0)
		{
			/* parentKey is exactly in this backend, so add it! */
			elektraSplitAppend (split, kdb->split->handles[i], keyDup(kdb->split->parents[i]), kdb->split->syncbits[i]);
		}
		else if (keyRel(parentKey, kdb->split->parents[i]) >= 0)
		{
			/* this backend is completely below the parentKey, so lets add it. */
			elektraSplitAppend (split, kdb->split->handles[i], keyDup(kdb->split->parents[i]), kdb->split->syncbits[i]);
		}
	}

	return 1;
}



/**
 * Splits up the keysets and search for a sync bit.
 *
 * It does not check if there were removed keys,
 * see elektraSplitRemove() for the next step.
 *
 * It does not create new backends, this has to be
 * done by buildup before.
 *
 * @pre elektraSplitBuildup() need to be executed before.
 *
 * @param split the split object to work with
 * @param handle to get information where the individual keys belong
 * @param ks the keyset to divide
 *
 * @return 0 if there were no sync bits
 * @return 1 if there were sync bits
 * @return -1 if no backend was found for a key
 * @ingroup split
 */
int elektraSplitDivide (Split *split, KDB *handle, KeySet *ks)
{
	ssize_t curFound = 0; /* If key could be appended to any of the existing split keysets */
	int needsSync = 0;
	Key *curKey = 0;
	Backend *curHandle = 0;

	ksRewind (ks);
	while ((curKey = ksNext (ks)) != 0)
	{
		curHandle = elektraMountGetBackend(handle, curKey);
		if (!curHandle) return -1;

		curFound = elektraSplitSearchBackend(split, curHandle, curKey);

		if (curFound == -1) continue; // key not relevant in this kdbSet

		ksAppendKey (split->keysets[curFound], curKey);
		if (keyNeedSync(curKey) == 1)
		{
			split->syncbits[curFound] |= 1;
			needsSync = 1;
		}
	}

	return needsSync;
}


/**
 * Appoints all keys from ks to yet unsynced splits.
 *
 * @pre elektraSplitBuildup() need to be executed before.
 *
 * @param split the split object to work with
 * @param handle to determine to which backend a key belongs
 * @param ks the keyset to appoint to split
 *
 * @return 1 on success
 * @return -1 if no backend was found for a key
 * @ingroup split
 */
int elektraSplitAppoint (Split *split, KDB *handle, KeySet *ks)
{
	ssize_t curFound = 0; /* If key could be appended to any of the existing split keysets */
	Key *curKey = 0;
	Backend *curHandle = 0;
	ssize_t defFound = elektraSplitAppend (split, 0, 0, 0);

	ksRewind (ks);
	while ((curKey = ksNext (ks)) != 0)
	{
		curHandle = elektraMountGetBackend(handle, curKey);
		if (!curHandle) return -1;

		curFound = elektraSplitSearchBackend(split, curHandle, curKey);

		if (curFound == -1) curFound = defFound;

		if (split->syncbits[curFound] & 1)
		{
			continue;
		}

		ksAppendKey (split->keysets[curFound], curKey);
	}

	return 1;
}

static void elektraDropCurrentKey(KeySet *ks,
		Key *warningKey,
		const Backend *curHandle,
		const char *msg)
{
	const Key *k = ksCurrent(ks);

	const size_t sizeOfStaticText = 100;
	char *warningMsg = elektraMalloc(
			keyGetNameSize(curHandle->mountpoint) +
			keyGetValueSize(curHandle->mountpoint) +
			keyGetNameSize(k) +
			strlen(msg) +
			sizeOfStaticText);
	strcpy(warningMsg, "drop key ");
	const char *name = keyName(k);
	if (name) strcat(warningMsg, name);
	else strcat(warningMsg, "(no name)");
	strcat(warningMsg, " not belonging to ");
	strcat(warningMsg, keyName(curHandle->mountpoint));
	strcat(warningMsg, " with name ");
	strcat(warningMsg, keyString(curHandle->mountpoint));
	strcat(warningMsg, " because ");
	strcat(warningMsg, msg);
	ELEKTRA_ADD_WARNING(79, warningKey, warningMsg);
	elektraFree(warningMsg);
	cursor_t c = ksGetCursor(ks);
	keyDel (ksPopAtCursor(ks, c));
	ksSetCursor(ks, c);
	ksPrev(ks); // next ksNext() will point correctly again
}

/**
 * Does some work after getting of backends is finished.
 *
 * @pre elektraSplitAppoint() needs to be executed before.
 *
 * - check if keys are in correct backend
 * - remove syncbits
 * - update sizes in the backends
 *
 * @param split the split object to work with
 * @param warningKey postcondition violations are reported here
 * @param handle the handle to preprocess the keys
 * @return 1 on success
 * @return -1 if no backend was found for a key or split->parents
 *         has invalid namespace
 * @ingroup split
 */
int elektraSplitGet (Split *split, Key *warningKey, KDB *handle)
{
	Key *cur = 0;
	Backend *curHandle = 0;

	/* Dont iterate the default split part */
	for (size_t i=0; i<split->size-1; ++i)
	{
		if (test_bit(split->syncbits[i], 0))
		{
			/* Dont process keysets which come from the user
			   and not from the backends */
			continue;
		}

		ksRewind (split->keysets[i]);
		while ((cur = ksNext(split->keysets[i])) != 0)
		{
			curHandle = elektraMountGetBackend(handle, cur);
			if (!curHandle) return -1;

			keyClearSync (cur);

			if (curHandle != split->handles[i])
			{
				elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it is hidden by other mountpoint");
			}
			else switch (keyGetNamespace(cur))
			{
			case KEY_NS_SPEC:
				if (!keyIsSpec(split->parents[i]))
					elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it is not spec");
				break;
			case KEY_NS_DIR:
				if (!keyIsDir(split->parents[i]))
					elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it is not dir");
				break;
			case KEY_NS_USER:
				if (!keyIsUser(split->parents[i]))
					elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it is not user");
				break;
			case KEY_NS_SYSTEM:
				if (!keyIsSystem(split->parents[i]))
					elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it is not system");
				break;
			case KEY_NS_PROC:
				elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it has a proc key name");
				break;
			case KEY_NS_EMPTY:
				elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it has an empty name");
				break;
			case KEY_NS_META:
				elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it has a meta name");
				break;
			case KEY_NS_CASCADING:
				elektraDropCurrentKey(split->keysets[i], warningKey, curHandle, "it has a cascading name");
				break;
			case KEY_NS_NONE:
				return -1;
			}
		}

		/* Update sizes */
		switch (keyGetNamespace(split->parents[i]))
		{
		case KEY_NS_SPEC:
			split->handles[i]->specsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_DIR:
			split->handles[i]->dirsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_USER:
			split->handles[i]->usersize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_SYSTEM:
			split->handles[i]->systemsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_PROC:
		case KEY_NS_EMPTY:
		case KEY_NS_META:
		case KEY_NS_CASCADING:
		case KEY_NS_NONE:
			return -1;
		}
	}

	return 1;
}

/**
 * Also update sizes after kdbSet() to recognize multiple kdbSet() attempts.
 *
 * @warning cant use the same code with elektraSplitGet because there is
 * no default split part for kdbSet().
 */
int elektraSplitUpdateSize (Split *split)
{
	/* Iterate everything */
	for (size_t i=0; i<split->size; ++i)
	{
		switch (keyGetNamespace(split->parents[i]))
		{
		case KEY_NS_SPEC:
			split->handles[i]->specsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_DIR:
			split->handles[i]->dirsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_USER:
			split->handles[i]->usersize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_SYSTEM:
			split->handles[i]->systemsize = ksGetSize(split->keysets[i]);
			break;
		case KEY_NS_PROC:
		case KEY_NS_EMPTY:
		case KEY_NS_NONE:
		case KEY_NS_META:
		case KEY_NS_CASCADING:
			return -1;
		}
	}
	return 1;
}

/**
 * Merges together all parts of split into dest.
 *
 * @param split the split object to work with
 * @param dest the destination keyset where all keysets are appended.
 * @return 1 on success
 * @ingroup split
 */
int elektraSplitMerge (Split *split, KeySet *dest)
{
	/* Iterate everything */
	for (size_t i=0; i<split->size; ++i)
	{
		ksAppend (dest, split->keysets[i]);
	}
	return 1;
}

/** Add sync bits everywhere keys were removed.
 *
 * Only this function can really decide if sync is needed or not.
 *
 * @pre split needs to be processed with elektraSplitDivide() before.
 *
 * @return 0 if kdbSet() is not needed
 * @return 1 if kdbSet() is needed
 * @return -1 on wrong split->parents
 * @pre user/system was split before.
 * @param split the split object to work with
 * @ingroup split
 *
**/
int elektraSplitSync(Split *split)
{
	int needsSync = 0;

	for (size_t i=0; i<split->size; ++i)
	{
		if (split->syncbits[i] & 1)
		{
			needsSync = 1;
			continue;
		}

		switch (keyGetNamespace(split->parents[i]))
		{
		case KEY_NS_SPEC:
			/* Check for spec keyset for removed keys */
			if (split->handles[i]->specsize != ksGetSize(split->keysets[i]))
			{
				split->syncbits[i] |= 1;
				needsSync = 1;
			}
			break;
		case KEY_NS_DIR:
			/* Check for dir keyset for removed keys */
			if (split->handles[i]->dirsize != ksGetSize(split->keysets[i]))
			{
				split->syncbits[i] |= 1;
				needsSync = 1;
			}
			break;
		case KEY_NS_USER:
			/* Check for user keyset for removed keys */
			if (split->handles[i]->usersize != ksGetSize(split->keysets[i]))
			{
				split->syncbits[i] |= 1;
				needsSync = 1;
			}
			break;
		case KEY_NS_SYSTEM:
			/* Check for system keyset for removed keys */
			if (split->handles[i]->systemsize != ksGetSize(split->keysets[i]))
			{
				split->syncbits[i] |= 1;
				needsSync = 1;
			}
			break;
		case KEY_NS_PROC:
		case KEY_NS_EMPTY:
		case KEY_NS_META:
		case KEY_NS_CASCADING:
		case KEY_NS_NONE:
			return -1;
		}
	}

	return needsSync;
}

/** Prepares for kdbSet() mainloop afterwards.
 *
 * All splits which do not need sync are removed and a deep copy
 * of the remaining keysets is done.
 *
 * @param split the split object to work with
 * @ingroup split
 * @retval 0 on success
 */
int elektraSplitPrepare (Split *split)
{
	size_t size = split->size;
	for (size_t i=0; i<size; ++i)
	{
		if ((split->syncbits[i] & 1) == 1)
		{
			KeySet *n = ksDeepDup(split->keysets[i]);
			ksDel (split->keysets[i]);
			split->keysets[i] = n;
			continue;
		}

		if ((split->syncbits[i] & 1) == 0)
		{
			/* We don't need i anymore */
			for (size_t j = i+1; j<size; ++j)
			{
				if ((split->syncbits[j] & 1) == 1)
				{
					Backend *tmpBackend = 0;
					Key *tmpParent = 0;
					int tmpSyncbits = 0;

					/* Ohh, we have found an important j... let's swap j and i */
					ksDel (split->keysets[i]);
					split->keysets[i] = ksDup(split->keysets[j]);

					tmpBackend = split->handles[i];
					split->handles[i] = split->handles[j];
					split->handles[j] = tmpBackend;

					tmpParent = split->parents[i];
					split->parents[i] = split->parents[j];
					split->parents[j] = tmpParent;

					tmpSyncbits = split->syncbits[i];
					split->syncbits[i] = split->syncbits[j];
					split->syncbits[j] = tmpSyncbits;
					goto cont; /* Continue the outer loop */
				}
			}

			/* We are finished, search did not find anything which needed sync, so let's remove the rest */
			for (size_t j = i; j<size; ++j)
			{
				if ((split->syncbits[j] & 1) == 0)
				{
					ksDel (split->keysets[j]);
					keyDecRef (split->parents[j]);
					keyDel (split->parents[j]);

					split->keysets[j]=0;
					split->handles[j]=0;
					split->parents[j]=0;
					split->syncbits[j]=0;
					--split->size;
				}
			}
			break;
		}
cont: ;
	}

	return 0;
}

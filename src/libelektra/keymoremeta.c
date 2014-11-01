#include <kdb.h>
#include <kdbconfig.h>
#include <kdbprivate.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif



/*********************************************
 *       UID, GID access methods             *
 *********************************************/




/**
 * Get the user ID of a key.
 *
 * @deprecated This API is obsolete.
 *
 * @section UID UID
 *
 * The user ID is a unique identification for every user present on a
 * system. Keys will belong to root (0) as long as you did not get their
 * real UID with kdbGet().
 *
 * Although usually the same, the UID of a key is not related to its owner.
 *
 * A fresh key will have no UID.
 *
 * @param key the key object to work with
 * @return the system's UID of the key
 * @return (uid_t)-1 on NULL key
 * @see keyGetGID(), keySetUID(), keyGetOwner()
 * @ingroup keymeta
 */
uid_t keyGetUID(const Key *key)
{
	const char *uid;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (uid_t)-1;

	uid = keyValue(keyGetMeta(key, "uid"));
	if (!uid) return (uid_t)-1;
	if (*uid == '\0') return (uid_t)-1;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(uid, &endptr, 10);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == uid) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return (uid_t)-1;
}



/**
 * Set the user ID of a key.
 *
 * @deprecated This API is obsolete.
 *
 * See @ref UID for more information about user IDs.
 *
 * @param key the key object to work with
 * @param uid the user ID to set
 * @return 0 on success
 * @return -1 on NULL key or conversion error
 * @see keySetGID(), keyGetUID(), keyGetOwner()
 * @ingroup keymeta
 */
int keySetUID(Key *key, uid_t uid)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%d", uid) < 0)
	{
		return -1;
	}

	keySetMeta(key, "uid", str);

	return 0;
}



/**
 * Get the group ID of a key.
 *
 * @deprecated This API is obsolete.
 *
 * @section GID GID
 *
 * The group ID is a unique identification for every group present on
 * a system. Keys will belong to root (0) as long as you did not get their
 * real GID with kdbGet().
 *
 * Unlike UID users might change their group. This makes it possible to
 * share configuration between some users.
 *
 * A fresh key will have (gid_t)-1 also known as the group nogroup.
 * It means that the key is not related to a group ID at the moment.
 *
 * @param key the key object to work with
 * @return the system's GID of the key
 * @return (gid_t)-1 on NULL key or currently unknown ID
 * @see keySetGID(), keyGetUID()
 * @ingroup keymeta
 */
gid_t keyGetGID(const Key *key)
{
	const char *gid;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (gid_t)-1;

	gid = keyValue(keyGetMeta(key, "gid"));
	if (!gid) return (gid_t)-1;
	if (*gid == '\0') return (gid_t)-1;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(gid, &endptr, 10);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == gid) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return (gid_t)-1;
}



/**
 * Set the group ID of a key.
 *
 * @deprecated This API is obsolete.
 *
 * See @ref GID for more information about group IDs.
 *
 * @param key the key object to work with
 * @param gid is the group ID
 * @return 0 on success
 * @return -1 on NULL key
 * @see keyGetGID(), keySetUID()
 * @ingroup keymeta
 */
int keySetGID(Key *key, gid_t gid)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%d", gid) < 0)
	{
		return -1;
	}

	keySetMeta(key, "gid", str);

	return 0;
}




/**
 * Set mode so that key will be recognized as directory.
 *
 * @deprecated This API is obsolete.
 *
 * The function will add all executable bits.
 *
 * - Mode 0200 will be translated to 0311
 * - Mode 0400 will be translated to 0711
 * - Mode 0664 will be translated to 0775
 *
 * The macro KDB_DIR_MODE (defined to 0111) will be used for that.
 *
 * The executable bits show that child keys are allowed and listable. There
 * is no way to have child keys which are not listable for anyone, but it is
 * possible to restrict listing the keys to the owner only.
 *
 * - Mode 0000 means that it is a key not read or writable to anyone.
 * - Mode 0111 means that it is a directory not read or writable to anyone.
 *   But it is recognized as directory to anyone.
 *
 * For more about mode see keySetMode().
 *
 * It is not possible to access keys below a not executable key.
 * If a key is not writeable and executable kdbSet() will fail to access the
 * keys below.
 * If a key is not readable and executable kdbGet() will fail to access the
 * keys below.
 *
 * @param key the key to set permissions to be recognized as directory.
 * @return 0 on success
 * @return -1 on NULL pointer
 * @see keySetMode()
 * @ingroup keymeta
 */
int keySetDir(Key *key)
{
	mode_t mode;
	if (!key) return -1;

	mode = keyGetMode(key);
	mode |= KDB_DIR_MODE;
	keySetMode(key, mode);

	return 0;
}



/**
 * Return the key mode permissions.
 *
 * @deprecated This API is obsolete.
 *
 * Default is 0664 (octal) for keys and 0775 for directory keys
 * which used keySetDir().
 *
 * The defaults are defined with the macros KDB_FILE_MODE and KDB_DIR_MODE.
 *
 * For more information about the mode permissions see @ref mode.
 *
 * @param key the key object to work with
 * @return mode permissions of the key
 * @return KDB_FILE_MODE as defaults
 * @return (mode_t)-1 on NULL pointer
 * @see keySetMode()
 * @ingroup keymeta
 */
mode_t keyGetMode(const Key *key)
{
	const char *mode;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (mode_t)-1;

	mode = keyValue(keyGetMeta(key, "mode"));
	if (!mode) return KDB_FILE_MODE;
	if (*mode == '\0') return KDB_FILE_MODE;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(mode, &endptr, 8);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == mode) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return KDB_FILE_MODE;
}



/**
 * Set the key mode permissions.
 *
 * @deprecated This API is obsolete.
 * It is only a mapping
 * to keySetMeta(key, "mode", str) which should be prefered.
 *
 * The mode consists of 9 individual bits for mode permissions.
 * In the following explanation the octal notation with leading
 * zero will be used.
 *
 * Default is 0664 (octal) for keys and 0775 for directory keys
 * which used keySetDir().
 *
 * The defaults are defined with the macros KDB_FILE_MODE and KDB_DIR_MODE.
 *
 * @note libelektra 0.7.0 only allows 0775 (directory keys) and
 * 0664 (other keys). More will be added later in a sense of the
 * description below.
 *
 * @section mode Modes
 *
 * 0000 is the most restrictive mode. No user might read, write
 * or execute the key.
 *
 * Reading the key means to get the value by kdbGet().
 *
 * Writing the key means to set the value by kdbSet().
 *
 * Execute the key means to make a step deeper in the hierarchy.
 * But you must be able to read the key to be able to list the
 * keys below. See also keySetDir() in that context.
 * But you must be able to write the key to be able to add or
 * remove keys below.
 *
 * 0777 is the most relaxing mode. Every user is allowed to
 * read, write and execute the key, if he is allowed to execute
 * and read all keys below.
 *
 * 0700 allows every action for the current user, identified by
 * the uid. See keyGetUID() and keySetUID().
 *
 * To be more specific for the user the single bits can elect
 * the mode for read, write and execute. 0100 only allows
 * executing which gives the information that it is a directory
 * for that user, but not accessable. 0200 only allows reading.
 * This information may be combined to 0300, which allows execute
 * and reading of the directory. Last 0400 decides about the
 * writing permissions.
 *
 * The same as above is also valid for the 2 other octal digits.
 * 0070 decides about the group permissions, in that case full
 * access. Groups are identified by the gid. See keyGetGID() and
 * keySetGID(). In that example everyone with a different uid,
 * but the gid of the the key, has full access.
 *
 * 0007 decides about the world permissions. This is taken into
 * account when neighter the uid nor the gid matches. So that
 * example would allow everyone with a different uid and gid
 * of that key gains full access.
 *
 * @param key the key to set mode permissions
 * @param mode the mode permissions
 * @return 0 on success
 * @return -1 on NULL key
 * @see keyGetMode()
 * @ingroup keymeta
 */
int keySetMode(Key *key, mode_t mode)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%o", mode) < 0)
	{
		return -1;
	}

	keySetMeta(key, "mode", str);

	return 0;
}


/*********************************************
 *    Access times methods                   *
 *********************************************/


/**
 * Get last time the key data was read from disk.
 *
 * @deprecated This API is obsolete.
 *
 * Every kdbGet() might update the access time
 * of a key. You get information when the key
 * was read the last time from the database.
 *
 * You will get 0 when the key was not read already.
 *
 * Beware that multiple copies of keys with keyDup() might have different
 * atimes because you kdbGet() one, but not the
 * other. You can use this information to decide which
 * key is the latest.
 *
 * @param key Key to get information from.
 * @return the time you got the key with kdbGet()
 * @return 0 on key that was never kdbGet()
 * @return (time_t)-1 on NULL pointer
 * @see keySetATime()
 * @see kdbGet()
 * @ingroup keymeta
 */
time_t keyGetATime(const Key *key)
{
	const char *atime;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (time_t)-1;

	atime = keyValue(keyGetMeta(key, "atime"));
	if (!atime) return 0;
	if (*atime == '\0') return (time_t)-1;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(atime, &endptr, 10);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == atime) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return (time_t)-1;
}

/**
 * Update the atime information for a key.
 *
 * @deprecated This API is obsolete.
 *
 * When you do manual sync of keys you might also
 * update the atime to make them indistinguishable.
 *
 * It can also be useful if you work with
 * keys not using a keydatabase.
 *
 * @param key The Key object to work with
 * @param atime The new access time for the key
 * @return 0 on success
 * @return -1 on NULL pointer
 * @see keyGetATime()
 * @ingroup keymeta
 */
int keySetATime(Key *key, time_t atime)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%lu", atime) < 0)
	{
		return -1;
	}

	keySetMeta(key, "atime", str);

	return 0;
}


/**
 * Get last modification time of the key on disk.
 *
 * @deprecated This API is obsolete.
 *
 * You will get 0 when the key was not read already.
 *
 * Everytime you change value or comment and kdbSet()
 * the key the mtime will be updated. When you kdbGet()
 * the key, the atime is set appropriate.
 *
 * Not changed keys may not even passed to kdbSet_backend()
 * so it will not update this time, even after kdbSet().
 *
 * It is possible that other keys written to disc
 * influence this time if the backend is not grained
 * enough.
 *
 * If you add or remove a key the key thereunder
 * in the hierarchy will update the mtime
 * if written with kdbSet() to disc.
 *
 * @param key Key to get information from.
 * @see keySetMTime()
 * @return the last modification time
 * @return (time_t)-1 on NULL pointer
 * @ingroup keymeta
 */
time_t keyGetMTime(const Key *key)
{
	const char *mtime;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (time_t)-1;

	mtime = keyValue(keyGetMeta(key, "mtime"));
	if (!mtime) return 0;
	if (*mtime == '\0') return (time_t)-1;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(mtime, &endptr, 10);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == mtime) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return (time_t)-1;
}

/**
 * Update the mtime information for a key.
 *
 * @deprecated This API is obsolete.
 *
 * @param key The Key object to work with
 * @param mtime The new modification time for the key
 * @return 0 on success
 * @see keyGetMTime()
 * @ingroup keymeta
 */
int keySetMTime(Key *key, time_t mtime)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%lu", mtime) < 0)
	{
		return -1;
	}

	keySetMeta(key, "mtime", str);

	return 0;
}


/**
 * Get last time the key metadata was changed from disk.
 *
 * @deprecated This API is obsolete.
 *
 * You will get 0 when the key was not read already.
 *
 * Any changed field in metadata will influence the
 * ctime of a key.
 *
 * This time is not updated if only value
 * or comment are changed.
 *
 * Not changed keys will not update this time,
 * even after kdbSet().
 *
 * It is possible that other keys written to disc
 * influence this time if the backend is not grained
 * enough.
 *
 * @param key Key to get information from.
 * @see keySetCTime()
 * @return (time_t)-1 on NULL pointer
 * @return the metadata change time
 * @ingroup keymeta
 */
time_t keyGetCTime(const Key *key)
{
	const char *ctime;
	long int val;
	char *endptr;
	int errorval = errno;

	if (!key) return (time_t)-1;

	ctime = keyValue(keyGetMeta(key, "ctime"));
	if (!ctime) return 0;
	if (*ctime == '\0') return (time_t)-1;

	/*From now on we have to leave using cleanup*/
	errno = 0;
	val = strtol(ctime, &endptr, 10);

	/*Check for errors*/
	if (errno) goto cleanup;

	/*Check if nothing was found*/
	if (endptr == ctime) goto cleanup;

	/*Check if the whole string was processed*/
	if (*endptr != '\0') goto cleanup;

	return val;
cleanup:
	/*First restore errno*/
	errno = errorval;
	return (time_t)-1;
}


/**
 * Update the ctime information for a key.
 *
 * @deprecated This API is obsolete.
 *
 * @param key The Key object to work with
 * @param ctime The new change metadata time for the key
 * @return 0 on success
 * @return -1 on NULL pointer
 * @see keyGetCTime()
 * @ingroup keymeta
 */
int keySetCTime(Key *key, time_t ctime)
{
	char str[MAX_LEN_INT];
	if (!key) return -1;

	if (snprintf (str, MAX_LEN_INT-1, "%lu", ctime) < 0)
	{
		return -1;
	}

	keySetMeta(key, "ctime", str);

	return 0;
}


/**
 * Check if the mode for the key has access
 * privileges.
 *
 * In the filesys backend a key represented through
 * a file has the mode 664, but a key represented
 * through a folder 775. keyIsDir() checks if all
 * 3 executeable bits are set.
 *
 * If any executable bit is set it will be recognized
 * as a directory.
 *
 * @note keyIsDir may return true even though you
 *   can't access the directory.
 *
 * To know if you can access the directory, you
 * need to check, if your
 * - user ID is equal the key's
 *   user ID and the mode & 100 is true
 * - group ID is equal the key's
 *   group ID and the mode & 010 is true
 * - mode & 001 is true
 *
 * Accessing does not mean that you can get any value or
 * comments below, see @ref mode for more information.
 *
 * @note currently mountpoints can only where keyIsDir()
 *       is true (0.7.0) but this is likely to change.
 *
 * @param key the key object to work with
 * @return 1 if key is a directory, 0 otherwise
 * @return -1 on NULL pointer
 * @see keySetDir(), keySetMode()
 * @ingroup keytest
 */
int keyIsDir(const Key *key)
{
	mode_t mode;

	if (!key) return -1;

	mode = keyGetMode(key);

	return ((mode & KDB_DIR_MODE) != 0);
}



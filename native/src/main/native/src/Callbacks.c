#include <unistd.h>
#include <jni.h>
#include <stdint.h>
#include <libspotify/api.h>
#include <string.h>
#include <stdlib.h>

#include "Callbacks.h"
#include "JahSpotify.h"
#include "JNIHelpers.h"
#include "ThreadHelpers.h"
#include "Logging.h"

extern void populateJAlbumInstanceFromAlbumBrowse(JNIEnv *env, sp_album *album, sp_albumbrowse *albumBrowse, jobject albumInstance);
extern void populateJArtistInstanceFromArtistBrowse(JNIEnv *env, sp_artistbrowse *artistBrowse, jobject artist);
extern jobject createJLinkInstance(JNIEnv *env, sp_link *link);
extern jobject createJPlaylistInstance(JNIEnv *env, sp_link* link, const char* name, sp_link* image);

extern sp_session *g_sess;

extern jobject g_connectionListener;
extern jobject g_playbackListener;
extern jobject g_searchCompleteListener;
extern jobject g_mediaLoadedListener;

extern jclass g_playbackListenerClass;
extern jclass g_connectionListenerClass;
extern jclass g_searchCompleteListenerClass;
extern jclass g_nativeSearchResultClass;
extern jclass g_mediaLoadedListenerClass;

jint addObjectToCollection(JNIEnv *env, jobject collection, jobject object) {
	jclass clazz;
	jmethodID methodID;

	clazz = (*env)->GetObjectClass(env, collection);
	if (clazz == NULL) return 1;

	methodID = (*env)->GetMethodID(env, clazz, "add", "(Ljava/lang/Object;)Z");
	if (methodID == NULL) return 1;

	// Invoke the method
	(*env)->CallBooleanMethod(env, collection, methodID, object);
	if (checkException(env) != 0) {
		log_error("callbacks", "addObjectToCollection", "Exception while adding object to collection");
	}

	return 0;
}

void startPlaybackSignalled() {
//	JNIEnv* env = NULL;
//	int result;
//	jclass aClass;
//	jmethodID method;
//	jstring nextUriStr;
//	char *nextUri;
//
//     log_debug("callbacks","startPlaybackSignalled","About to start pre-loading track");
//     
//         
//     if (!retrieveEnv((JNIEnv*)&env))
//     {
//         goto fail;
//     }
//     
//     method = (*env)->GetMethodID(env, g_playbackListenerClass, "nextTrackToPreload", "()Ljava/lang/String;");
//     
//     if (method == NULL)
//     {
//         log_error("callbacks","startPlaybackSignalled","Could not load callback method string nextTrackToPreload() on class PlaybackListener");
//         goto fail;
//     }
//     
//     nextUriStr = (*env)->CallObjectMethod(env, g_playbackListener, method);
//     checkException(env);
//     
//     if (nextUriStr)
//     {
//         nextUri = ( uint8_t * ) ( *env )->GetStringUTFChars ( env, nextUriStr, NULL );
//         
//         sp_link *link = sp_link_create_from_string(nextUri);
//         
//         if (link)
//         {
//             sp_track *track = sp_link_as_track(link);
//             sp_link_release(link);
//             sp_error error = sp_session_player_prefetch(g_sess,track);
//             sp_track_release(track);
//             if (error != SP_ERROR_OK)
//             {
//                 log_error("callbacks","startPlaybackSignalled","Error prefetch: %s",sp_error_message(error));
//                 goto fail;
//             }
//         }
//     }
//     
//     goto exit;
//     
//     fail:
//     log_error("callbacks","startPlaybackSignalled","Error during callback");
//     
//     exit:
//     
//     if (nextUri) 
//     {
//         (*env)->ReleaseStringUTFChars(env, nextUriStr,nextUri);
//     }
}

int signalConnected() {
	JNIEnv* env = NULL;
	jmethodID method;

	if (!g_connectionListener) {
		log_error("jahspotify", "signalConnected", "No connection listener registered");
		return 1;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_connectionListenerClass, "connected", "()V");

	if (method == NULL) {
		log_error("callbacks", "signalConnected", "Could not load callback method connected() on class ConnectionListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_connectionListener, method);
	checkException(env);

	goto exit;

	fail: log_error("callbacks", "signalConnected", "Error during callback");

	exit: detachThread();

	return 0;
}

int signalInitialized(int initialized) {
	JNIEnv* env = NULL;
	jmethodID method;

	if (!g_connectionListener) {
		log_error("jahspotify", "signalInitialized", "No connection listener registered");
		return 1;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_connectionListenerClass, "initialized", "(Z)V");

	if (method == NULL) {
		log_error("callbacks", "signalInitialized", "Could not load callback method initialized() on class ConnectionListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_connectionListener, method, initialized == 1 ? JNI_TRUE : JNI_FALSE);
	checkException(env);

	goto exit;

	fail: log_error("callbacks", "signalInitialized", "Error during callback");

	exit: detachThread();

	return 0;
}

int signalDisconnected() {
	JNIEnv* env = NULL;
	jmethodID method;

	if (!g_connectionListener) {
		log_error("jahspotify", "signalDisconnected", "No connection listener registered");
		return 1;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_connectionListenerClass, "disconnected", "()V");

	if (method == NULL) {
		log_error("callbacks", "signalDisconnected", "Could not load callback method connected() on class ConnectionListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_connectionListener, method);
	checkException(env);

	goto exit;

	fail: log_error("callbacks", "signalDisconnected", "Error during callback");

	exit: detachThread();

	return 0;
}

int signalLoggedOut() {
	JNIEnv* env = NULL;
	if (!retrieveEnv((JNIEnv*) &env)) {
		log_info("callbacks", "signalLoggedOut", "Error during callback");
	} else {
		invokeVoidMethod(env, g_connectionListener, "loggedOut");
		log_info("callbacks", "signalLoggedOut", "Logout signalled");
	}
	detachThread();
	return 0;
}

int signalLoggedIn(int loggedIn) {
	JNIEnv* env = NULL;
	jmethodID method;

	if (!g_connectionListener) {
		log_error("jahspotify", "signalLoggedIn", "No connection listener registered");
		return 1;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_connectionListenerClass, "loggedIn", "(Z)V");

	if (method == NULL) {
		log_error("callbacks", "signalLoggedIn", "Could not load callback method loggedIn() on class ConnectionListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_connectionListener, method, loggedIn == 1 ? JNI_TRUE : JNI_FALSE);
	if (checkException(env) != 0) {
		log_error("callbacks", "signalLoggedIn", "Exception while calling listener");
		goto fail;
	}

	goto exit;

	fail: log_error("callbacks", "signalLoggedIn", "Error during callback");

	exit: detachThread();
	return 0;
}

int signalPlaylistsLoaded() {
	JNIEnv* env = NULL;
	if (!retrieveEnv((JNIEnv*) &env)) {
		log_error("callbacks", "signalPlaylistsLoaded", "Error sending signal about playlists loaded.");
		detachThread();
		return -1;
	}
	invokeVoidMethod(env, g_connectionListener, "playlistsLoaded");
	return 0;
}

void signalBlobUpdated(const char* blob) {
	JNIEnv* env = NULL;
	jmethodID method;
	jstring blobStr = NULL;

	if (!g_connectionListener) {
		log_error("jahspotify", "signalLoggedIn", "No connection listener registered");
		return;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_connectionListenerClass, "blobUpdated", "(Ljava/lang/String;)V");
	if (method == NULL) {
		log_error("callbacks", "signalBlobUpdated", "Could not load callback method blobUpdated() on class ConnectionListener");
		goto fail;
	}

	blobStr = (*env)->NewStringUTF(env, blob);

	(*env)->CallVoidMethod(env, g_connectionListener, method, blobStr);
	if (checkException(env) != 0) {
		log_error("callbacks", "signalLoggedIn", "Exception while calling listener");
		goto fail;
	}

	goto exit;

	fail: log_error("callbacks", "signalLoggedIn", "Error during callback");

	exit:

	if (blobStr) (*env)->DeleteLocalRef(env, blobStr);

	detachThread();
}

int signalTrackEnded(char *uri, bool forcedTrackEnd) {
	if (!g_playbackListener) {
		log_error("jahspotify", "signalTrackEnded", "No playback listener");
		return 1;
	}

	JNIEnv* env = NULL;
	jmethodID method;
	jstring uriStr;

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	if (uri) {
		uriStr = (*env)->NewStringUTF(env, uri);
		if (uriStr == NULL) {
			log_error("callbacks", "signalTrackEnded", "Error creating java string");
			goto fail;
		}
	}

	method = (*env)->GetMethodID(env, g_playbackListenerClass, "trackEnded", "(Ljava/lang/String;Z)V");

	if (method == NULL) {
		log_error("callbacks", "signalTrackEnded", "Could not load callback method trackEnded(string) on class jahnotify.PlaybackListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_playbackListener, method, uriStr, forcedTrackEnd);
	if (checkException(env) != 0) {
		log_error("callbacks", "signalTrackEnded", "Exception while calling callback");
		goto fail;
	}

	goto exit;

	fail: log_error("callbacks", "signalTrackEnded", "Error during callback\n");

	exit: if (uriStr) (*env)->DeleteLocalRef(env, uriStr);

	detachThread();
	return 0;
}

int signalTrackStarted(const char *uri) {
	JNIEnv* env = NULL;
	jmethodID method;
	jstring uriStr;

	log_debug("callbacks", "signalTrackStarted", "URI: %s", uri);
	if (!g_playbackListener) {
		log_error("callbacks", "signalTrackStarted", "No playback listener");
		return 1;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	if (uri) {
		uriStr = (*env)->NewStringUTF(env, uri);
		if (uriStr == NULL) {
			log_error("callbacks", "signalTrackStarted", "Error creating java string");
			goto fail;
		}
	}

	method = (*env)->GetMethodID(env, g_playbackListenerClass, "trackStarted", "(Ljava/lang/String;)V");

	if (method == NULL) {
		log_error("callbacks", "signalTrackStarted", "Could not load callback method trackStarted(string) on class jahnotify.PlaybackListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_playbackListener, method, uriStr);
	checkException(env);

	goto exit;

	fail: log_error("callbacks", "signalTrackStarted", "Error during callback");

	exit: if (uriStr) (*env)->DeleteLocalRef(env, uriStr);

	detachThread();
	return 0;
}

void signalPlayTokenLost() {
	JNIEnv* env = NULL;
	jmethodID method;

	if (!g_playbackListener) {
		log_error("callbacks", "signalPlayTokenLost", "No playback listener");
		return;
	}

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_playbackListenerClass, "playTokenLost", "()V");
	if (method == NULL) {
		log_error("callbacks", "signalPlayTokenLost", "Could not load callback method trackStarted() on class jahnotify.PlaybackListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_playbackListener, method);
	checkException(env);
	goto exit;

	fail: log_error("callbacks", "signalPlayTokenLost", "Error during callback");

	exit: detachThread();
}

int signalArtistBrowseLoaded(sp_artistbrowse *artistBrowse, jobject artistInstance) {
	JNIEnv* env = NULL;
	jmethodID aMethod;

	sp_link *artistLink = NULL;
	jclass jClass;

	log_debug("jahspotify", "signalArtistBrowseLoaded", "Artist browse loaded");

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	jClass = (*env)->FindClass(env, "jahspotify/media/Artist");
	if (jClass == NULL) {
		log_error("jahspotify", "createJArtistInstance", "Could not load jahnotify.media.Artist");
		goto fail;
	}

	if (!g_mediaLoadedListener) {
		log_error("jahspotify", "signalArtistBrowseLoaded", "No playlist media loaded listener registered");
		goto fail;
	}

	aMethod = (*env)->GetMethodID(env, g_mediaLoadedListenerClass, "artist", "(ILjahspotify/media/Artist;)V");

	if (aMethod == NULL) {
		log_error("callbacks", "signalArtistBrowseLoaded", "Could not load callback method artist(int,artist) on class NativeMediaLoadedListener");
		goto fail;
	}

	sp_artist *artist = sp_artistbrowse_artist(artistBrowse);
	if (!artist) {
		log_error("callbacks", "signalArtistBrowseLoaded", "Could not load artist from ArtistBrowse");
		goto fail;
	}

	sp_artist_add_ref(artist);

	artistLink = sp_link_create_from_artist(artist);

	sp_link_add_ref(artistLink);

	jobject artistJLink = createJLinkInstance(env, artistLink);

	setObjectObjectField(env, artistInstance, "id", "Ljahspotify/media/Link;", artistJLink);

	sp_link_release(artistLink);

	setObjectStringField(env, artistInstance, "name", sp_artist_name(artist));

	sp_artist_release(artist);

	// Convert the instance to an artist
	// Pass it up in the callback
	populateJArtistInstanceFromArtistBrowse(env, artistBrowse, artistInstance);

	invokeVoidMethod_Z(env, artistInstance, "setLoaded", JNI_TRUE);
	(*env)->CallVoidMethod(env, g_mediaLoadedListener, aMethod, 0, artistInstance);
	if (checkException(env) != 0) {
		log_error("callbacks", "signalArtistBrowseLoaded", "Exception while calling callback");
		goto fail;
	}

	goto exit;

	fail: log_error("jahspotify", "signalArtistBrowseLoaded", "Error occurred while processing callback");

	exit: (*env)->DeleteGlobalRef(env, artistInstance);
	if (artistBrowse) {
		sp_artistbrowse_release(artistBrowse);
	}
	detachThread();
	return 0;
}

int signalImageLoaded(sp_image *image, jobject imageInstance) {
	if (!g_mediaLoadedListener) {
		log_error("jahspotify", "signalImageLoaded", "No playlist media loaded listener registered");
		return 1;
	}

	JNIEnv* env = NULL;
	jmethodID method;

	log_debug("callbacks", "signalImageLoaded", "Image loaded: token: %d\n", 0);

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_mediaLoadedListenerClass, "image", "(ILjahspotify/media/Link;Ljahspotify/media/ImageSize;[B)V");

	if (method == NULL) {
		log_error("callbacks", "signalImageLoaded", "Could not load callback method image(Link) on class NativeMediaLoadedListener");
		goto fail;
	}

	sp_link *link = sp_link_create_from_image(image);
	sp_link_add_ref(link);
	jobject jLink = createJLinkInstance(env, link);
	sp_link_release(link);

	size_t size;
	const void* pData = sp_image_data(image, &size);
	jbyteArray byteArray = (*env)->NewByteArray(env, size);
	jboolean isCopy = 0;
	jbyte* pByteData = (*env)->GetByteArrayElements(env, byteArray, &isCopy);
	size_t i;
	for (i = 0; i < size; i++)
		pByteData[i] = ((byte*) pData)[i];
	(*env)->ReleaseByteArrayElements(env, byteArray, pByteData, 0);
	setObjectObjectField(env, imageInstance, "bytes", "[B", byteArray);
	(*env)->DeleteLocalRef(env, byteArray);

	invokeVoidMethod_Z(env, imageInstance, "setLoaded", JNI_TRUE);
	(*env)->CallVoidMethod(env, g_mediaLoadedListener, method, 0, jLink, NULL, NULL);

	if (checkException(env) != 0) {
		log_error("callbacks", "signalImageLoaded", "Exception while calling listener");
		goto fail;
	}

	log_debug("callbacks", "signalImageLoaded", "Callback invokved");

	goto exit;

	fail:

	exit:

	(*env)->DeleteGlobalRef(env, imageInstance);
	sp_image_release(image);
	detachThread();

	return 0;
}

int signalPlaylistLoaded(jobject playlist) {
	if (!g_mediaLoadedListener) {
		log_error("jahspotify", "signalPlaylistLoaded", "No playlist media loaded listener registered");
		return 1;
	}

	JNIEnv* env = NULL;
	jmethodID method;

	log_debug("jahspotify", "signalPlaylistLoaded", "Playlist loaded");

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	method = (*env)->GetMethodID(env, g_mediaLoadedListenerClass, "playlist", "(Ljahspotify/media/Playlist;)V");
	if (method == NULL) {
		log_error("callbacks", "signalPlaylistLoaded", "Could not load callback method playlist(Link) on class NativeMediaLoadedListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_mediaLoadedListener, method, playlist);
	log_debug("callbacks", "signalPlaylistLoaded", "Callback invokved");

	goto exit;

	fail:

	exit: detachThread();
	return 0;
}

int signalAlbumBrowseLoaded(sp_albumbrowse *albumBrowse, jobject albumInstance) {
	JNIEnv* env = NULL;
	jmethodID aMethod;

	sp_album *album = NULL;
	sp_link *albumLink = NULL;
	jclass jClass;

	if (!g_mediaLoadedListener) {
		log_error("jahspotify", "signalAlbumBrowseLoaded", "No album media loaded listener registered");
		goto fail;
	}

	log_debug("jahspotify", "signalAlbumBrowseLoaded", "Albumbrowse loaded");

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	jClass = (*env)->FindClass(env, "jahspotify/media/Album");
	if (jClass == NULL) {
		log_error("jahspotify", "signalAlbumBrowseLoaded", "Could not load jahnotify.media.Album");
		goto fail;
	}

	aMethod = (*env)->GetMethodID(env, g_mediaLoadedListenerClass, "album", "(ILjahspotify/media/Album;)V");

	if (aMethod == NULL) {
		log_error("callbacks", "signalAlbumBrowseLoaded", "Could not load callback method album(int,album) on class NativeMediaLoadedListener");
		goto fail;
	}

	album = sp_albumbrowse_album(albumBrowse);

	if (!album) {
		log_error("callbacks", "signalAlbumBrowseLoaded", "Could not load album from AlbumBrowse");
		goto fail;
	}
	sp_album_add_ref(album);

	albumLink = sp_link_create_from_album(album);

	sp_link_add_ref(albumLink);

	jobject albumJLink = createJLinkInstance(env, albumLink);

	setObjectObjectField(env, albumInstance, "id", "Ljahspotify/media/Link;", albumJLink);

	setObjectStringField(env, albumInstance, "name", sp_album_name(album));

	// Convert the instance to an artist
	// Pass it up in the callback
	populateJAlbumInstanceFromAlbumBrowse(env, album, albumBrowse, albumInstance);

	(*env)->CallVoidMethod(env, g_mediaLoadedListener, aMethod, 0, albumInstance);
	invokeVoidMethod_Z(env, albumInstance, "setLoaded", JNI_TRUE);

	if (checkException(env) != 0) {
		log_error("callbacks", "signalAlbumBrowseLoaded", "Exception while calling callback");
		goto fail;
	}

	goto exit;

	fail:

	exit: (*env)->DeleteGlobalRef(env, albumInstance);
	if (albumLink) {
		sp_link_release(albumLink);
	}

	if (album) {
		sp_album_release(album);
	}
	if (albumBrowse) {
		sp_albumbrowse_release(albumBrowse);
	}
	detachThread();
	return 0;
}

// int signalTrackLoaded(sp_track *track, int32_t token)
// {
//   if (!g_mediaLoadedListener)
//   {
//       log_error("jahspotify","signalTrackLoaded","No playlist media loaded listener registered");
//       return 1;
//   }
//   
//   JNIEnv* env = NULL;
//   jmethodID method;
//   
//   log_debug("callbacks","signalTrackLoaded","Track loaded: token: %d", token);
//   
//   if (!retrieveEnv((JNIEnv*)&env))
//   {
//       goto fail;
//   }
//   
//   method = (*env)->GetMethodID(env, g_mediaLoadedListenerClass, "track", "(ILjahspotify/media/Link;)V");
//   
//   if (method == NULL)
//   {
//       log_error("callbacks","signalTrackLoaded","Could not load callback method track(Link) on class NativeMediaLoadedListener");
//       goto fail;
//   }
//   
//   sp_link *link = sp_link_create_from_track(track,0);
//   
//   sp_link_add_ref(link);
//   
//   jobject jLink = createJLinkInstance(env,link);
//   
//   sp_link_release(link);
//   
//   (*env)->CallVoidMethod(env,g_mediaLoadedListener,method,token,jLink);
//   if (checkException(env) != 0)
//   {
//       log_error("callbacks","signalTrackLoaded","Exception while calling listener");
//       goto fail;
//   }
//   
//   log_debug("callbacks","signalTrackLoaded","Callback invokved");
//   goto exit;
//   
//   fail:
//   
//   exit:
//   
//   sp_track_release(track);
// }

jobject createSearchResult(JNIEnv* env) {
	return createInstanceFromJClass(env, g_nativeSearchResultClass);
}

void signalToplistComplete(sp_toplistbrowse *result, jobject nativeSearchResult) {
	sp_toplistbrowse_add_ref(result);
	JNIEnv* env = NULL;
	jobject jLink;
	jobject trackLinkCollection;
	jobject albumLinkCollection;
	jobject artistLinkCollection;

	int numResultsFound = 0;
	int index = 0;

	log_debug("jahspotify", "signalToplistComplete", "Search complete: token: %d");

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	trackLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "tracksFound", "Ljava/util/List;", trackLinkCollection);

	numResultsFound = sp_toplistbrowse_num_tracks(result);
	for (index = 0; index < numResultsFound; index++) {
		sp_track *track = sp_toplistbrowse_track(result, index);
		if (track && sp_track_get_availability(g_sess, track) == SP_TRACK_AVAILABILITY_AVAILABLE) {
			sp_track_add_ref(track);

			if (sp_track_is_loaded(track)) {
				sp_link *link = sp_link_create_from_track(track, 0);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, trackLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalToplistComplete", "Track not loaded");
			}
			sp_track_release(track);
		}
	}
	if (trackLinkCollection) (*env)->DeleteLocalRef(env, trackLinkCollection);

	albumLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "albumsFound", "Ljava/util/List;", albumLinkCollection);

	numResultsFound = sp_toplistbrowse_num_albums(result);
	for (index = 0; index < numResultsFound; index++) {
		sp_album *album = sp_toplistbrowse_album(result, index);
		if (album && sp_album_is_available(album)) {
			sp_album_add_ref(album);

			if (sp_album_is_loaded(album)) {
				sp_link *link = sp_link_create_from_album(album);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, albumLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalToplistComplete", "Album not loaded");
			}
			sp_album_release(album);
		}
	}
	if (albumLinkCollection) (*env)->DeleteLocalRef(env, albumLinkCollection);

	artistLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "artistsFound", "Ljava/util/List;", artistLinkCollection);

	numResultsFound = sp_toplistbrowse_num_artists(result);
	for (index = 0; index < numResultsFound; index++) {
		sp_artist *artist = sp_toplistbrowse_artist(result, index);
		if (artist) {
			sp_artist_add_ref(artist);

			if (sp_artist_is_loaded(artist)) {
				sp_link *link = sp_link_create_from_artist(artist);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, artistLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalToplistComplete", "Artist not loaded");
			}
			sp_artist_release(artist);
		}
	}
	if (artistLinkCollection) (*env)->DeleteLocalRef(env, artistLinkCollection);

	invokeVoidMethod_Z(env, nativeSearchResult, "setLoaded", JNI_TRUE);

	goto exit;

	fail:

	exit: sp_toplistbrowse_release(result);
	(*env)->DeleteGlobalRef(env, nativeSearchResult);
	detachThread();
}

int signalSearchComplete(sp_search *search, int32_t token) {
	if (!g_searchCompleteListener) {
		log_error("jahspotify", "signalSearchComplete", "No playlist media loaded listener registered");
		return 1;
	}

	sp_search_add_ref(search);
	JNIEnv* env = NULL;
	jmethodID method;
	jobject jLink;
	jobject nativeSearchResult;
	jobject trackLinkCollection;
	jobject albumLinkCollection;
	jobject artistLinkCollection;
	jobject playlistLinkCollection;
	int numResultsFound = 0;
	int index = 0;

	log_debug("jahspotify", "signalSearchComplete", "Search complete: token: %d", token);

	if (!retrieveEnv((JNIEnv*) &env)) {
		goto fail;
	}

	// Create the Native Search Result instance
	nativeSearchResult = createInstanceFromJClass(env, g_nativeSearchResultClass);

	trackLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "tracksFound", "Ljava/util/List;", trackLinkCollection);

	numResultsFound = sp_search_num_tracks(search);
	for (index = 0; index < numResultsFound; index++) {
		sp_track *track = sp_search_track(search, index);
		if (track && sp_track_get_availability(g_sess, track) == SP_TRACK_AVAILABILITY_AVAILABLE) {
			sp_track_add_ref(track);

			if (sp_track_is_loaded(track)) {
				sp_link *link = sp_link_create_from_track(track, 0);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, trackLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalSearchComplete", "Track not loaded");
			}

			sp_track_release(track);

		}
	}
	if (trackLinkCollection) (*env)->DeleteLocalRef(env, trackLinkCollection);

	albumLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "albumsFound", "Ljava/util/List;", albumLinkCollection);

	numResultsFound = sp_search_num_albums(search);
	for (index = 0; index < numResultsFound; index++) {
		sp_album *album = sp_search_album(search, index);
		if (album && sp_album_is_available(album)) {
			sp_album_add_ref(album);

			if (sp_album_is_loaded(album)) {
				sp_link *link = sp_link_create_from_album(album);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, albumLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalSearchComplete", "Album not loaded");
			}

			sp_album_release(album);

		}
	}
	if (albumLinkCollection) (*env)->DeleteLocalRef(env, albumLinkCollection);

	artistLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "artistsFound", "Ljava/util/List;", artistLinkCollection);

	numResultsFound = sp_search_num_artists(search);
	for (index = 0; index < numResultsFound; index++) {
		sp_artist *artist = sp_search_artist(search, index);
		if (artist) {
			sp_artist_add_ref(artist);

			if (sp_artist_is_loaded(artist)) {
				sp_link *link = sp_link_create_from_artist(artist);
				if (link) {
					sp_link_add_ref(link);
					jLink = createJLinkInstance(env, link);
					addObjectToCollection(env, artistLinkCollection, jLink);
					sp_link_release(link);
				}
			} else {
				log_error("jahspotify", "signalSearchComplete", "Artist not loaded");
			}

			sp_artist_release(artist);

		}
	}
	if (artistLinkCollection) (*env)->DeleteLocalRef(env, artistLinkCollection);

	playlistLinkCollection = createInstance(env, "java/util/ArrayList");
	setObjectObjectField(env, nativeSearchResult, "playlistsFound", "Ljava/util/List;", playlistLinkCollection);

	numResultsFound = sp_search_num_playlists(search);
	for (index = 0; index < numResultsFound; index++) {
		sp_link *link = sp_link_create_from_string(sp_search_playlist_uri(search, index));
		sp_link *imageLink = sp_link_create_from_string(sp_search_playlist_image_uri(search, index));

		jLink = createJPlaylistInstance(env, link, sp_search_playlist_name(search, index), imageLink);
		addObjectToCollection(env, playlistLinkCollection, jLink);

		if (link) sp_link_release(link);
		if (imageLink) sp_link_release(imageLink);
	}
	if (playlistLinkCollection) (*env)->DeleteLocalRef(env, playlistLinkCollection);

	setObjectIntField(env, nativeSearchResult, "totalNumTracks", sp_search_total_tracks(search));
	setObjectIntField(env, nativeSearchResult, "trackOffset", sp_search_num_tracks(search));

	setObjectIntField(env, nativeSearchResult, "totalNumAlbums", sp_search_total_albums(search));
	setObjectIntField(env, nativeSearchResult, "albumOffset", sp_search_num_albums(search));

	setObjectIntField(env, nativeSearchResult, "totalNumArtists", sp_search_total_artists(search));
	setObjectIntField(env, nativeSearchResult, "artistOffset", sp_search_num_artists(search));

	setObjectIntField(env, nativeSearchResult, "totalNumPlaylists", sp_search_total_playlists(search));
	setObjectIntField(env, nativeSearchResult, "playlistOffset", sp_search_num_playlists(search));

	setObjectStringField(env, nativeSearchResult, "query", sp_search_query(search));
	setObjectStringField(env, nativeSearchResult, "didYouMean", sp_search_did_you_mean(search));

	method = (*env)->GetMethodID(env, g_searchCompleteListenerClass, "searchCompleted", "(ILjahspotify/SearchResult;)V");

	if (method == NULL) {
		log_error("jahspotify", "signalSearchComplete", "Could not load callback method searchCompleted() on class SearchListener");
		goto fail;
	}

	(*env)->CallVoidMethod(env, g_searchCompleteListener, method, token, nativeSearchResult);
	if (checkException(env) != 0) {
		log_error("jahspotify", "signalSearchComplete", "Exception while calling search complete listener");
		goto fail;
	}

	goto exit;

	fail:

	exit: sp_search_release(search);
	detachThread();
	return 0;
}

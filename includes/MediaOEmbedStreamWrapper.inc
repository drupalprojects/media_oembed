<?php

/**
 *  @file
 *  Extends the MediaReadOnlyStreamWrapper class to handle oEmbed resources.
 */

/**
 *  Create an instance like this:
 *  $oembed = new MediaOEmbedStreamWrapper('oembed://[provider-url]');
 */
class MediaOEmbedStreamWrapper extends MediaReadOnlyStreamWrapper {
  static function getMimeType($uri, $mapping = NULL) {
    $url = rawurldecode(substr($uri, 9));
    $embed = media_oembed_get_data($url);

    // The mime type can be specified in hook_media_oembed_response_alter()
    // which is useful to map responses with type 'rich' and 'link' to more
    // appropriate Drupal file entity bundles.
    // See media_oembed_media_oembed_response_alter().
    if (isset($embed['mime_type'])) {
      return $embed['mime_type'];
    }

    if ($embed) {
      switch ($embed['type']) {
        case 'video':
          return 'video/oembed';
        case 'photo':
          return 'image/oembed';
        default:
          return 'text/oembed';
      }
    }
    else {
      // URIs for valid oEmbed responses may become invalid after they are saved
      // to the file_managed table. This might happen because the oEmbed
      // endpoint is down or the provider is misconfigured. The content may
      // have been deleted or become inaccessible. Some of these
      // situations are temporary, so the stream wrapper should try to return a
      // MIME type for URIs that are already saved as Drupal file entities.
      $type = db_select('file_managed', 'f')
        ->fields('f', array('type'))
        ->condition('uri', $uri)
        ->execute()
        ->fetchField();

      if (in_array($type, array('image', 'video', 'audio'))) {
        return $type .'/oembed';
      }
      else if ($type) {
        return 'text/oembed';
      }
    }

    return FALSE;
  }

  public function getExternalUrl() {
    return rawurldecode(substr($this->getUri(), 9));
  }

  function getOriginalThumbnailPath() {
    $uri = $this->getUri();
    $url = drupal_realpath($uri);

    $data = media_oembed_get_data($url);

    if (isset($data['thumbnail_url'])) {
      return $data['thumbnail_url'];
    }
    else {
      return FALSE;
    }
  }

  function getLocalThumbnailPath() {
    $uri = $this->getUri();
    $url = drupal_realpath($uri);
    $local_path = file_stream_wrapper_uri_normalize('public://oembed/'. $url);

    if (!file_exists($local_path)) {
      $dirname = drupal_dirname($local_path);
      file_prepare_directory($dirname, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);
      $original_thumbnail_path = $this->getOriginalThumbnailPath();

      if ($original_thumbnail_path) {
        $response = drupal_http_request($original_thumbnail_path);

        if (!isset($response->error)) {
          file_unmanaged_save_data($response->data, $local_path, TRUE);
        }
        else {
          @copy($original_thumbnail_path, $local_path);
        }
      }
      else {
        return FALSE;
      }
    }

    return $local_path;
  }
}

<?php
/**
 * Downloads a packaged artifacts from a Nexus repository. Might also work with other Maven 
 * repositories. Switches between curl and wget, make sure one of them is installed.
 * 
 * The reason i created this is because we build modules and themes and version them using maven, 
 * deploying them on Nexus using release management. We use public and private packages and need 
 * to make a secure connection for those artifacts that are secured. Also Nexus redirects requests
 * rendering the filename incorrect using wget or curl. Using the correct wget en curl commands 
 * makes Nexus work nicely. Now we can use snapshots and release versions of our projects and also
 * build a complete drupal stack using maven-release plugin.
 * 
 * Example make setup:
 * 
 
	core = 6.31
	api = 2

	; CUSTOM PROJECT/MODULE/THEME/LIBRARY ETC
	projects[NAMEOFMODULE][type] = "module"
	projects[NAMEOFMODULE][download][type] = "nexus"
	projects[NAMEOFMODULE][download][url] = "https://[YOURNEXUSHOST_URL]/service/local/artifact/maven/redirect"
	projects[NAMEOFMODULE][download][username] = "USERNAME" ;(optional, needed for private artifacts)
	projects[NAMEOFMODULE][download][password] = "PASSWORD" ;(optional, needed for private artifacts)
	projects[NAMEOFMODULE][download][repo] = "releases" ;(releases|snapshosts)
	projects[NAMEOFMODULE][download][group] = "ARTIFACT GROUP"
	projects[NAMEOFMODULE][download][artifact] = "ARTIFACT NAME"
	projects[NAMEOFMODULE][download][version] = "ARTIFACT VERSION" ;(1.0.0|1.0.0-SNAPSHOT
	projects[NAMEOFMODULE][download][packaging] = "ARTIFACT PACKAGING" ;(tar.gz|zip|jar|pom)

 *
 *
 * @author Sebastiaan Blommers
 * 
 * @return mixed
 *   The download location on success, FALSE otherwise.
 */
function make_download_nexus($name, $type, $download, $download_location) {
  $tmp_path = make_tmp();
  $tmp_location = drush_tempdir() . '/' . basename($download_location);
  
  if (!empty($download['url'])) {
    static $use_wget;
    if ($use_wget === NULL)
      $use_wget = drush_shell_exec('which wget'); 

    $secure = !empty($download['username']) && !empty($download['password']);
    $args = array();
    if($secure) {
    	$args[] = $download['username'];
		$args[] = $download['password'];
    }
    $args[] = $tmp_location;
    $args[] = sprintf('%s?r=%s&g=%s&a=%s&v=%s&p=%s', $download['url'], $download['repo'], $download['group'], $download['artifact'], $download['version'], $download['packaging']);

    $command = 'curl ';
    if($secure) // add username|password arguments if set
		$command.='-u %s:%s ';
		
    $command.='--fail -s -L --connect-timeout 30 -o %s %s';

    if ($use_wget) {
		// override command with wget (preferred)
		$command = 'wget ';
		if($secure) // add username|password arguments if set
			$command.='--http-user=%s --http-password=%s  ';
			
		$command.='-q --timeout=30 -O %s %s';
    }
    array_unshift($args, $command);
    // everything is gathered, now execute over drush
    if (call_user_func_array('drush_shell_exec', $args)) {
      drush_log(dt('@project @version downloaded from N.E.X.U.S. repository.', array('@project' => $name, '@version' => $download['version'] )), 'ok');
      
      $download_filename = isset($download['artifact']) ? $download['artifact'] : '';
      $subtree = isset($download['subtree']) ? $download['subtree'] : NULL;
      return make_download_file_unpack($tmp_location, $download_location, $download_filename, $subtree);
    }
  }
  else {
    $download['url'] = dt("unspecified location");
  }
  make_error('DOWNLOAD_ERROR', dt('Unable to download @project from Nexus using @url. Make sure the artifact exists.', array('@project' => $name, '@url' => $args[3])));
  drush_delete_dir(dirname($tmp_location), TRUE);
  return FALSE;
}

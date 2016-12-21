<?php

define('RBKC_API_DEFAULT_REQUESTER_UID', 1);

class APIUserNotFoundException extends Exception {}

function _rbkc_api_access_retrieve() {
	return TRUE;
}

function _rbkc_api_needs_review_retrieve($sitename = '') {
  $callback_name = '_rbkc_api_workbench_get_needs_review_count';
  drupal_alter('needs_review_callback', $callback_name); // Generate hook_needs_review_callback_alter
  return $callback_name($sitename);
}

/* WORKBENCH RESOURCE */

function _rbkc_api_workbench_get_needs_review_count() {
  $query = db_select('workbench_moderation_node_history', 'wmnh')
    ->fields('wmnh');
  $query->condition('wmnh.state', "needs_review"); // only want 'needs_review' status
  $query->condition('wmnh.current', 1);  // where needs_review is the CURRENT status (table stores history)
  $results = $query->execute();
   
  return $results->rowCount(); // return the number of rows found
}


/* USER RESOURCE */

function _rbkc_api_user_create($user_data) {
	global $user;
	$q_item = array(
		'worker_include_file' => array(
			'type' => 'inc',
			'module' => 'rbkc_api',
			'name' => 'resources/rbkc_api_resources',
		),
		'worker_callback' => 'rbkc_api_user_create_queue_worker',
		'requester_uid' => $user->uid,
		'request_data' => $user_data,
	);
	drupal_alter('rbkc_api_queue_item', $q_item);
	rbkc_api_queue_add($q_item);
	return 'Queued user creation for: '.$user_data['username'].' <'.$user_data['email'].'>';
}

function _rbkc_api_user_update($username, $user_data) {
  $q_item = array(
    'worker_include_file' => array(
      'type' => 'inc',
      'module' => 'rbkc_api',
      'name' => 'resources/rbkc_api_resources',
    ),
    'worker_callback' => 'rbkc_api_user_update_queue_worker',
    'requester_uid' => isset($user_data['requester_uid'])? $user_data['requester_uid'] : RBKC_API_DEFAULT_REQUESTER_UID,
    'request_data' => $user_data,
    'target_username' => $username,
  );
  drupal_alter('rbkc_api_queue_item', $q_item);
  rbkc_api_queue_add($q_item);
  return 'Queued user update for: '.$user_data['username'].' <'.$user_data['email'].'>';
}

/********************************/

/**
 * User create callback. This is run when an incoming user-create request
 * is being picked up from the queue and processed.
 * @param array $item The queue item being processed.
 * @return null
 */
function rbkc_api_user_create_queue_worker($item) {
  if (rbkc_api_user_exists($item['request_data']['username'], $item['request_data']['email'])) {
    watchdog('rbkc_api_user_create_queue_worker', 'Inbound REST create-user request ignored because user already exist, for username '.$item['request_data']['username'].' with email '.$item['request_data']['email']);
    return true;
  }
  
	$fields = array(
		'name' => $item['request_data']['username'],
		'mail' => $item['request_data']['email'],
		'pass' => user_password(8),
		'status' => 1,
		'init' => 'email_address',
		'roles' => _rbkc_api_get_roles($item['request_data']['roles']),
		'field_full_name' => array(
			LANGUAGE_NONE => array(
				0 => array('value' => $item['request_data']['fullname']),
			),
		),
		'data' => array('creator_uid' => $item['requester_uid']),		
	);
  
	// Call hook_rbkc_api_user_pre_create for pre-save 
	module_invoke_all('rbkc_api_user_pre_create', $fields, $item);

	$account = user_save(null, $fields);

	// Call hook_rbkc_api_user_pre_create for post-save 
	module_invoke_all('rbkc_api_user_post_create', $account, $item);
}

/**
 * User update callback. This is run when an incoming user-update request
 * is being picked up from the queue and processed.
 * @param array $item The queue item being processed.
 * @return null
 */
function rbkc_api_user_update_queue_worker($item) {
  if (!isset($item['request_data']['target_username'])) {
    throw new APIUserNotFoundException("Target username was not provided in REST user update request");
  }
  
  $target_username = $item['request_data']['target_username'];
  $target_user_obj = user_load_by_name($target_username);
  
  if (empty($target_user_obj)) {
    throw new APIUserNotFoundException('Received API request update user '.$target_username.' but username is not found.');
  }

  $save_data = array();
  foreach ($item['request_data'] as $field => $value) {
    switch ($field) {
      case 'fullname':
        $save_data['field_full_name'][LANGUAGE_NONE][0]['value'] = $value;
        break;
      case 'name':
        $save_data['name'] = $value;
        break;
      case 'mail':
        $save_data['mail'] = $value;
        break;
      case 'status':
        $save_data['status'] = $value;
        break;
    }
  }
  if ($save_data) {
    user_save($target_user_obj, $save_data);
  }
}


/**
 * Translates an array of rol names into array of rid => role_name
 * @param  array $roles 1-dimension array of role names
 * @return array
 */
function _rbkc_api_get_roles($roles) {
	$rid_roles = array(DRUPAL_AUTHENTICATED_RID => 'authenticated user');
	$sitewide_roles = user_roles(true); // get all roles on the site
	foreach ($sitewide_roles as $swr_rid => $swr_name) {
		if (in_array($swr_name, $roles)) {
			$rid_roles[$swr_rid] = $swr_name;
		}
	}
	return $rid_roles;
}
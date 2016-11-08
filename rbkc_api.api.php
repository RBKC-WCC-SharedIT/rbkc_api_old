<?php
// This module exposes the following hooks...


/**
 * Add, remove or amend items to be added to the RBKC API job queue.
 * These values will be available in hook_rbkc_api_user_pre_create()
 * and hook_rbkc_api_user_post_create().
 * @param  array $queue_item The queue item
 */
function hook_rbkc_api_queue_item_alter(&$queue_item) {

}

/**
 * Called just before a new user i about to be created
 * @param  array $data Contains all the data to be used for creating the user
 * @param  array $item The queue item retrieved for this job
 */
function hook_rbkc_api_user_pre_create($data, $queue_item) {

}

/**
 * Called after a new user has just been created
 * @param  obj $user The new user that was just created
 * @param  array $item The queue item retrieved for this job
 */
function hook_rbkc_api_user_post_create($user, $queue_item) {
	
}
<?php

/**
 * Devel Generate batch handling functions using the BatchAPI
 */

/**
 * Batch creation based on bundle generation form submission.
 */
function eck_generate_batch_bundles($form_state) {
  $operations = array();
  // Setup the batch operations and save the variables.
  $operations[] = array('eck_generate_batch_bundles_pre_bundle', array($form_state['values']));
  // Add the delete bundles operation if it was called for.
  if ($form_state['values']['kill_bundles']) {
    $operations[] = array('eck_generate_batch_bundles_kill', array($form_state['values']));
  }
  // Setup the batch operation to create the bundles.
  $operations[] = array('eck_generate_batch_bundles_add_bundle',array($form_state['values']));
  // Setup the batch array with the previously added operations.
  $batch = array(
    'title' => t('Generating Products'),
    'operations' => $operations,
    'finished' => 'eck_generate_batch_finished',
    'file' => drupal_get_path('module', 'eck_generate') . '/devel_generate_batch.inc',
  );
  // Trigger the batch process.
  batch_set($batch);
}

/**
 * Function for trigger batch generation of orders
 *
 * @TODO: Review for improvements and better performance and memory usage.
 */
function eck_generate_batch_orders($form_state) {
  $batch = array(
    'title' => t('Generating Orders'),
    'operations' => array(),
    'finished' => 'eck_generate_batch_orders_finished',
    'file' => drupal_get_path('module', 'eck_generate') . '/devel_generate_batch.inc',
  );

  if ($form_state['values']['kill_orders']) {
    $batch['operations'][] = array('eck_generate_batch_orders_kill', array($form_state['values']['kill_transactions'], $form_state['values']['kill_customer_profiles']));
  }

  $batch['operations'][] = array('eck_generate_batch_orders_add', array($form_state['values']));

  batch_set($batch);
}

/**
 * Initial function called on batch. Establishes basic variables.
 */
function eck_generate_batch_bundles_pre_bundle($vars, &$context) {
  $context['results'] = $vars;
  $context['results']['start_time'] = time();
  module_load_include('inc', 'eck_generate', 'devel_generate');
  eck_generate_bundles_pre_bundle($context['results']);
}

/**
 * Batch kill function to delete all bundles.
 */
function eck_generate_batch_bundles_kill($values, &$context) {
  module_load_include('inc', 'eck_generate', 'devel_generate');

  // Initialize data on first loop through.
  if (!isset($context['results']['delete']['current'])) {
    // Get full list of bundles.
    $context['results']['delete']['bundle_ids'] = db_select('commerce_bundle', 'p')
      ->fields('p', array('bundle_id'))
      ->condition('type', $values['bundle_types'], 'IN')
      ->execute()->fetchCol();

    // Establish max count, and get a counter for value 0.
    $context['results']['delete']['current'] = 0;
    $context['results']['delete']['max'] = count($context['results']['delete']['bundle_ids']);
  }

  // Initialize bundles to delete array.
  $bundles_to_delete = array();

  // Gather ten items for deleting.
  for ($i = 1; $i <= 10; $i++) {
    // Gather ten items for deleting.
    if (isset($context['results']['delete']['bundle_ids'][$context['results']['delete']['current']])) {
      $bundles_to_delete[] = $context['results']['delete']['bundle_ids'][$context['results']['delete']['current']];
    }
    $context['results']['delete']['current']++;
  }

  // Delete the ten items.
  commerce_bundle_delete_multiple($bundles_to_delete);

  // Track deletion progress.
  $context['finished'] = $context['results']['delete']['current'] / $context['results']['delete']['max'];
}

/**
 * Creates bundle entities as part of the batch operation.
 */
function eck_generate_batch_bundles_add_bundle($vars, &$context) {
  // Initialize values for looping.
  if (!isset($context['results']['add']['current'])) {
    $context['results']['add']['current'] = 0;
    $context['results']['add']['max'] = $vars['num_bundles'];
  }
  // Loop ten times per batch cycle.
  for ($i = 1; $i <= 10; $i++) {
    // Check that current doesn't = max.
    if ($context['results']['add']['current'] != $context['results']['add']['max']) {
      module_load_include('inc', 'eck_generate', 'devel_generate');
      module_load_include('inc', 'devel_generate', 'devel_generate');
      eck_generate_bundle_add_bundle($context['results']);
      $context['results']['add']['current']++;
    }
  }
  // Track progress.
  $context['finished'] = $context['results']['add']['current'] / $context['results']['add']['max'];
}

/**
 * Completion call made when batch generate bundles is called.
 */
function eck_generate_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = t('Finished @num_bundle_ids bundles attempted to be created.', array('@num_bundle_ids' => $results['num_bundles']));
    // Indicate how much time has elapsed in the system log.
    $elapsed_time = time() - $results['start_time'];
    $elapsed_time_message = t('Commerce Devel Generate took @ second to generate % entities.', array('@' => $elapsed_time, '%' => $results['num_bundles']));
    watchdog('eck_generate', $elapsed_time_message);
  }
  else {
    $message = t('There was an error. Batch may not have finished properly.');
  }
  drupal_set_message($message);
}

/** 
 * Create Customer Profile.
 */
function eck_generate_batch_customer_profile($type, $uid) {
  $profile = commerce_customer_profile_new($type, $uid);
  
  $address_info = array();
  $address_info['name_line'] = ucwords(devel_generate_word(mt_rand(6, 12)));
  $address_info['first_name'] = ucwords(devel_generate_word(mt_rand(6, 12)));
  $address_info['last_name'] = ucwords(devel_generate_word(mt_rand(6, 12)));


  $address_info['thoroughfare'] = ucwords(devel_generate_word(mt_rand(6, 12)));
  $address_info['locality'] = ucwords(devel_generate_word(mt_rand(6, 12)));
  $address_info['country'] = array_rand(_addressfield_country_options_list());
  $address_info['postal_code'] = rand(00000, 99999);
  $address_info += addressfield_default_values();
  
  $instances = field_info_instances('commerce_customer_profile', $type);
  foreach ($instances as $name => $instance) {
    $info_field = field_info_field($name);
    if ($info_field['type'] == 'addressfield') {
      $values = $address_info;
      $values['data'] = serialize($values['data']);
      $profile->{$name}[LANGUAGE_NONE][] = $values;
    }
  }
  commerce_customer_profile_save($profile);
  return $profile;  
}
/**
 * Create Order Batch Functions:
 */

/**
 * Deletes all existing orders.
 *
 * @param $kill_transactions
 *    Boolean indicating that the order's transactions should be removed as well.
 */
function eck_generate_batch_orders_kill($kill_transactions, $kill_customer_profiles, &$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox']['orders'] = db_query('SELECT order_id FROM {commerce_order}')->fetchCol();
    $context['sandbox']['max'] = count($context['sandbox']['orders']);
    $context['sandbox']['progress'] = 0;
  }

  if (!empty($context['sandbox']['orders'])) {
    $order_ids = array_slice($context['sandbox']['orders'], 0, 100);
    $context['sandbox']['orders'] = array_slice($context['sandbox']['orders'], 100);

    commerce_order_delete_multiple($order_ids);

    if ($kill_transactions) {
      $transaction_ids = db_query('SELECT transaction_id FROM {commerce_payment_transaction} WHERE order_id IN (:ids)', array(':ids' => $order_ids))->fetchCol();

      commerce_payment_transaction_delete_multiple($transaction_ids);
    }
    
    if ($kill_customer_profiles) {
      $customer_profile_ids = db_query("SELECT commerce_customer_billing_profile_id FROM {field_data_commerce_customer_billing} WHERE entity_type = 'commerce_order' AND entity_id  IN (:ids)", array(':ids' => $order_ids))->fetchCol();
      
      commerce_customer_profile_delete_multiple($customer_profile_ids);
    }

    $context['sandbox']['progress'] += count($order_ids);
  }

  if (!empty($context['sandbox']['orders'])) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
  else {
    drupal_set_message(t('Deleted %count orders.', array('%count' => $context['sandbox']['max'])));
  }
}

/**
 * Generate new orders.
 *
 * $options
 *    Mixed array containing options retrieved through the Form API.
 */
function eck_generate_batch_orders_add($options, &$context) {
  module_load_include('inc', 'devel_generate');
  
  if (empty($context['sandbox'])) {
    // Options.
    $context['sandbox']['options'] = $options;
    $context['sandbox']['options']['start'] = mktime(0, 0, 0, $options['start']['month'], $options['start']['day'], $options['start']['year']);
    $context['sandbox']['options']['end'] = mktime(23, 59, 59, $options['end']['month'], $options['end']['day'], $options['end']['year']);

    // Entities used during generation.
    $query = new EntityFieldQuery();
    $results = $query->entityCondition('entity_type', 'commerce_bundle')->execute();

    $context['sandbox']['bundles'] = $results['commerce_bundle'];
    $context['sandbox']['users'] = array_merge(devel_get_users(), array('0'));

    // Progress of the generation.
    $context['sandbox']['max'] = $context['sandbox']['options']['amount'];
    $context['sandbox']['progress'] = 0;

    // Results.
    $context['results']['orders'] = array();
    $context['results']['transactions'] = array();
    $context['results']['customer_profiles'] = array();
  }

  $batch_size = min(50, $context['sandbox']['max'] - $context['sandbox']['progress']);

  for ($x = 0; $x < $batch_size; $x ++) {
    $uid = $context['sandbox']['users'][array_rand($context['sandbox']['users'])];
    $date = mt_rand($context['sandbox']['options']['start'], $context['sandbox']['options']['end']);
    $bundle_count = mt_rand($context['sandbox']['options']['min_bundles'], $context['sandbox']['options']['max_bundles']);

    $order = commerce_cart_order_new($uid);
    $order_wrapper = entity_metadata_wrapper('commerce_order', $order);

    for ($i = 0; $i < $bundle_count; $i ++) {
      $bundle_entity = $context['sandbox']['bundles'][array_rand($context['sandbox']['bundles'])];

      $bundle = commerce_bundle_load($bundle_entity->bundle_id);

      $line_item = commerce_bundle_line_item_new($bundle, 1);
      $line_item->order_id = $order->order_id;
      $line_item->created = $date;
      commerce_line_item_save($line_item);

      $order_wrapper->commerce_line_items[] = $line_item;
    }

    $order->created = $date;
    
    if ($context['sandbox']['options']['create_customer_profiles']) {
      $billing_profile = eck_generate_batch_customer_profile('billing', $uid);
      $shipping_profile = eck_generate_batch_customer_profile('shipping', $uid);

      $context['results']['customer_profiles'][] = $billing_profile;
      $context['results']['customer_profiles'][] = $shipping_profile;
    
      $order->commerce_customer_billing[LANGUAGE_NONE][0]['profile_id'] = $billing_profile->profile_id;
      $order->commerce_customer_shipping[LANGUAGE_NONE][0]['profile_id'] = $shipping_profile->profile_id;

    }

    $status = array_rand($context['sandbox']['options']['order_statuses']);

    if ($status != $order->status) {
      $order = commerce_order_status_update($order, $status, TRUE);
    }

    commerce_order_save($order);

    $context['results']['orders'][] = $order;

    if ($context['sandbox']['options']['create_transactions']) {
      $method = array_rand($context['sandbox']['options']['payment_method']);

      $transaction = commerce_payment_transaction_new($method, $order->order_id);
      $transaction->uid = $uid;
      $transaction->amount = $order->commerce_order_total[LANGUAGE_NONE][0]['amount'];
      $transaction->created = $order->created;
      $transaction->status = 'success';

      commerce_payment_transaction_save($transaction);

      $context['results']['transactions'][] = $transaction;
    }

    $context['sandbox']['progress'] ++;
  }

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

/**
 * Finished callback in Batch API for order generation.
 */
function eck_generate_batch_orders_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(format_plural(count($results['orders']), 'Created 1 order', 'Created @count orders'));

    if (count($results['transactions'])) {
      drupal_set_message(format_plural(count($results['transactions']), 'Created 1 transaction', 'Created @count transactions'));
    }
    
    if (count($results['customer_profiles'])) {
      drupal_set_message(format_plural(count($results['customer_profiles']), 'Created 1 customer profile', 'Created @count customer profiles'));
    }
  }
  else {
    drupal_set_message(t('Finished with an error.'), 'error');
  }
}

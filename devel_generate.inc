<?php

/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

function eck_generate_entities($form_state) {
  if (!empty($form_state['values']['kill_entities'])) {
    eck_generate_entities_kill($form_state['values']);
  }

  if (count($form_state['values']['bundle_types'])) {
    eck_generate_entities_pre_bundles($form_state['values']);
    $start = time();
    for ($i = 1; $i <= $form_state['values']['num_entities']; $i++) {
      // Get list of users we will use.
      eck_generate_entities_add_bundle($form_state['values']);
      if (function_exists('drush_log') && $i % drush_get_option('feedback', 1000) == 0) {
        $now = time();
        drush_log(dt('Completed !feedback entities (!rate entities/min)', array('!feedback' => drush_get_option('feedback', 1000), '!rate' => (drush_get_option('feedback', 1000) * 60) / ($now - $start))), 'ok');
        $start = $now;
      }
    }
  }

  devel_generate_set_message(format_plural($form_state['values']['num_entities'], '1 entitie created.', 'Finished creating @count entities'));
}

/**
 * Handle the commerce_devel_generate_content_form request to kill all of the
 * existing products.
 *
 * This is used by both the batch and non-batch branches of the code.
 *
 * @param $values
 *   array of options obtained from commerce_devel_generate_products_form.
 */
function eck_generate_entities_kill($values) {
  $query = new EntityFieldQuery();

  // \[(.*?)\]                         /\(([^)]+)\)/
  $query->entityCondition('entity_type', 'node')
          ->entityCondition('bundle', 'article')
          ->propertyCondition('status', NODE_PUBLISHED)
          ->fieldCondition('field_news_types', 'value', 'spotlight', '=')
          ->fieldCondition('field_photo', 'fid', 'NULL', '!=')
          ->fieldCondition('field_faculty_tag', 'tid', $value)
          ->fieldCondition('field_news_publishdate', 'value', $year . '%', 'like')
          ->fieldOrderBy('field_photo', 'fid', 'DESC')
          ->range(0, 10)
          ->addMetaData('account', user_load(1)); // Run the query as user 1.

  $result = $query->execute();

  foreach ($results as $result) {
    $product_ids[] = $result->product_id;
  }

  if (!empty($product_ids)) {
    commerce_product_delete_multiple($product_ids);
    drupal_set_message(t('Deleted %count products.', array('%count' => count($product_ids))));
  }
}

/**
 * Create one product. Used by both batch and non-batch code branches.
 *
 * @param $num
 *   array of options obtained from commerce_devel_generate_products_form.
 */
function eck_generate_entities_add_bundle(&$results) {
  $type = array_rand($results['product_types']);
  $product = commerce_product_new($type);nod

  // Insert new data:
  $product->title = devel_create_greeking(mt_rand(2, $results['title_length']), TRUE);

    // Save default parameters back into the $product object.
  $product->sku = commerce_devel_generate_generate_sku();
  $product->status = TRUE;
  // $product->language = $form_state['values']['language'];
  devel_generate_set_language($results, $product);

  $users = $results['users'];
  $product->uid = $users[array_rand($users)];

  // Notify field widgets.
  // field_attach_submit('commerce_product', $product, $form, $form_state);

  // A flag to let hook_productapi() implementations know that this is a generated product.
  $product->commerce_devel_generate = $results;

  // Populate fields on behalf of field.module
  $modules = array('commerce_price');
  foreach ($modules as $module) {
    module_load_include('inc', 'commerce_devel_generate', $module . '.devel_generate');
  }
  module_load_include('inc', 'devel_generate', 'devel_generate.fields');
  devel_generate_fields($product, 'commerce_product', $product->type);

  // See commerce_devel_generate_productapi() for actions that happen before and after this save.
  $product = commerce_product_save($product);
}

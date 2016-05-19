<?php

/**
 * @file
 *  Generate content, taxonomy, menu, and users via drush framework.
 */

/**
 * Implementation of hook_drush_command().
 */
function eck_generate_drush_command() {
  $items['generate-entities'] = array(
    'description' => 'Create entities.',
    'arguments' => array(
      'num_entities' => 'Number of entities to generate.',
    ),
    'options' => array(
      'kill' => 'Delete all entities before generating new ones.',
      'types' => 'A comma delimited list of product types to create. Defaults to product.',
      'length' => 'Max word length of titles.',
    ),
    'aliases' => array('gene'),
    'callback' => 'drush_eck_generate_entities',
  );
  return $items;
}


/**
 * Command callback. Generate a number of users.
 */
function drush_eck_generate_entities($num_entities = NULL) {
  if (drush_generate_is_number($num_entities) == FALSE) {
    return drush_set_error('DEVEL_GENERATE_INVALID_INPUT', t('Invalid number of entities.'));
  }
  drush_generate_include_devel();
  module_load_include('inc', 'eck_generate', 'devel_generate');
  $values = array(
    'kill_entities' => drush_get_option('kill'),
    'num_entities' => $num_entities,
    'product_types' => drupal_map_assoc(explode(',', drush_get_option('types', 'product'))),
    'title_length' => drush_get_option('length', 4),
  );
  eck_generate_entities(array('values' => $values));
  drush_log(t('Generated @number entities.', array('@number' => $num_entities)), 'success');
}

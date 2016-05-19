<?php

/**
 * @file eck_generate.controller.inc
 * Helper functions for generating entities.
 */
function _eck_generate_get_entities($form) {
  $entities = array();
  foreach ($form['bundles'] as $key => $entity_bundles) {
    if (strpos($key, "#") === false) {
      $entities[] = $key;
    }
  }

  return $entities;
}

function _eck_generate_get_bundles(&$form_state) {
  $bundles = array();
  foreach (_eck_generate_get_entities($form_state['complete form']) as $entity_name) {
    $bundles += $form_state['values'][$entity_name];
    unset($form_state['values'][$entity_name]);
  }

  return $bundles;
}

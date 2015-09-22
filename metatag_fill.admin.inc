<?php
/**
 * @file
 * Admin pages.
 */

/**
 * Form callback; displays the Metatag Fill settings form.
 *
 * @see metatag_fill_menu()
 */
function metatag_fill_config($form, &$form_state) {
  drupal_set_title(t('Fill Metatag Defaults'));
  ctools_include('export');
  $form['#submit'][] = 'metatag_fill_config_submit';
  $metatags_opts = array();

  foreach (metatag_get_info('tags') as $key => $info) {
    $metatags_opts[$key] = $info['label'];
  }

  $form['metatag_fill_opts'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Select Tags to Fill Defaults For')
  );

  $form['metatag_fill_opts']['metatag_fill_keys'] = array(
    '#type' => 'checkboxes',
    '#options' => $metatags_opts,
    '#default_value' => variable_get('metatag_fill_keys', array()),
  );

  $configs = ctools_export_crud_load_all('metatag_config');
  ksort($configs);

  $form['metatag_fill_config_opts'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Select Configuration Instances to Process'),
    '#description' => t('Only content entity configuration instances are available.')
  );

  $config_opts = array();
  foreach ($configs as $config) {
    $data = explode(':', $config->instance);

    if ($info = entity_get_info($data[0])) {
      $config_opts[$config->instance] = $config->instance;
    }
  }

  $form['metatag_fill_config_opts']['metatag_fill_config'] = array(
    '#default_value' => array(),
    '#type' => 'checkboxes',
    '#options' => $config_opts,
  );

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Extra Options'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );

  $form['options']['force_overwrite'] = array(
    '#type' => 'checkbox',
    '#title' => t('Overwrite existing metatags'),
    '#description' => t('<strong>Warning:</strong> This is irreversible and will permanently override any custom data added for the selected tags. Use with caution.'),
    '#default_value' => FALSE,
  );

  return system_settings_form($form);
}

/**
 * Submit handler; sets up batches of content to process.
 *
 * @see metatag_fill_config()
 */
function metatag_fill_config_submit($form, $form_state) {
  $config_values = $form_state['values']['metatag_fill_config'];

  foreach ($config_values as $k => $v) {
    if ($v) {
      metatag_fill_batch_set($v, $form_state['values']['metatag_fill_keys'], $form_state['values']['options']);
    }
  }
}

/**
 * Populate and set a batch for a given entity bundle.
 *
 * @param array $info
 *   An array containing an entity and bundle in the format "entity:bundle".
 * @param array $metatag_keys
 *   An array of metatag keys to process.
 * @param array $options
 *   An array of options to pass to the callback.
 */
function metatag_fill_batch_set($info, $metatag_keys, $options = array()) {
  $data = explode(':', $info);

  if ($info = entity_get_info($data[0])) {
    $q = new EntityFieldQuery();
    $q->entityCondition('entity_type', $data[0]);

    if (!empty($data[1])) {
      $q->entityCondition('bundle', $data[1]);
    }
    $res = $q->execute();

    if (isset($res[$data[0]])) {
      $eids = array_keys($res[$data[0]]);
      $operations = array();

      foreach ($eids as $eid) {
        $args = array(
          'eid' => $eid,
          'info' => $data,
          'tags' => $metatag_keys,
          'options' => $options,
        );
        $operations[] = array(
          'metatag_fill_process_item',
          array(
            $args,
            t('Operating on Entity ID: !eid', array('!eid' => $eid))
          ),
        );
      }

      $batch_definition = array(
        'operations' => $operations,
        'finished' => 'metatag_fill_batch_finish'
      );
      batch_set($batch_definition);
    }
  }
}

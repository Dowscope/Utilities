<?php
/*
Plugin Name:  Dow Modal
Plugin URI:   http://dowscopemedia.ca/#/plugins/wordpress/dowmodal
Description:  A simple modal plugin
Version:      1.1.3
Author:       DowScope Media 
Author URI:   http://dowscopemedia.ca
License:      GPL2
License URI:  https://www.gnu.org/licenses/gpl-2.0.html
*/

if ( is_admin() ) {
    // we are in admin mode
    require_once __DIR__ . '/admin/dowm-admin.php';
}

class DOWMPlugin {
    function __construct() {
        register_activation_hook(__FILE__, array($this, 'activate'));
        register_deactivation_hook(__FILE__, array($this, 'deactivate'));
        register_uninstall_hook(__FILE__, array($this, 'uninstall'));
        
        add_shortcode('dowm_sc', array($this, 'shortcode'));
        add_action('wp_loaded', array($this, 'html'), 2000, 1);
        add_action('wp_enqueue_scripts', array($this, 'add_styles'));
        add_action('wp_enqueue_scripts', array($this, 'add_scripts'));
    }

    // Using a shortcode
    function shortcode($atts) {
        $content = '<button class="dowm_trigger">Translate</button>';
        return $content;
    }

    function html() { 
    ?>
        <dialog class="dowm_modal">
            <div class="dowm_content">
                <p><?php echo get_option('dowm_text'); ?></p>
                <p><?php echo get_option('dowm_text2'); ?></p>
                <p><?php echo get_option('dowm_text3'); ?></p>
                <p><?php echo get_option('dowm_text4'); ?></p>
            </div>
            <div class="dowm_shortcode">
                <?php echo do_shortcode('[google-translator]'); ?>
            </div>
            <button class="dowm_cancel">Close</button>
        </dialog>
        <button class="dowm_static_trigger <?php
            if (get_option( 'dowm_staticTrigger', '1')){
                echo 'dowm_show';
            } else {
                echo 'dowm_hide';
            } ?>">
            Translate
        </button>
    <?php 
    }

    function add_styles() {
        wp_register_style('dowm_style', plugin_dir_url(__FILE__).'includes/dowm_styles.css');
        wp_enqueue_style('dowm_style');
    }

    function add_scripts() {
        wp_enqueue_script(
            'dowm_script',
            plugin_dir_url(__FILE__).'includes/dowm_functions.js',
            array('jquery'),
            '1.0.0',
            true
        );
    }

    function activate() {}
    function deactivate() {}
    function uninstall() {}
}

$dowmPlugin = new DOWMPlugin();

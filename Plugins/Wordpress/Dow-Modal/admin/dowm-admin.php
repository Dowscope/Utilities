<?php
class DOWMPluginAdmin {
    function __construct() {
        add_action('admin_menu', array($this, 'adminPage'));
        add_action('admin_init', array($this, 'settings'));
    }

    function settings() {
        // Big Text box
        // TODO: Make textarea show linebreaks
        add_settings_section('dowm-primary-section', null, null, 'dowm-settings');
        add_settings_field( 'dowm_text', 'Public 1st Paragragh', array($this, 'textHTML'), 'dowm-settings', 'dowm-primary-section');
        register_setting('dowm','dowm_text',array('sanitize_callback' => 'sanitize_text_field','default' => '0'));
        add_settings_field( 'dowm_text2', 'Public 2nd Paragragh', array($this, 'textHTML2'), 'dowm-settings', 'dowm-primary-section');
        register_setting('dowm','dowm_text2',array('sanitize_callback' => 'sanitize_text_field','default' => '0'));
        add_settings_field( 'dowm_text3', 'Public 3rd Paragragh', array($this, 'textHTML3'), 'dowm-settings', 'dowm-primary-section');
        register_setting('dowm','dowm_text3',array('sanitize_callback' => 'sanitize_text_field','default' => '0'));
        add_settings_field( 'dowm_text4', 'Public 4th Paragragh', array($this, 'textHTML4'), 'dowm-settings', 'dowm-primary-section');
        register_setting('dowm','dowm_text4',array('sanitize_callback' => 'sanitize_text_field','default' => '0'));
        // TODO: Add Dynamic Shortcode
    }

    function textHTML() { ?>
        <textarea name="dowm_text" id="desc_text" cols="90" rows="5"><?php echo get_option('dowm_text') ?></textarea>
    <?php }
    function textHTML2() { ?>
        <textarea name="dowm_text2" id="desc_text2" cols="90" rows="5"><?php echo get_option('dowm_text2') ?></textarea>
    <?php }
    function textHTML3() { ?>
        <textarea name="dowm_text3" id="desc_text3" cols="90" rows="5"><?php echo get_option('dowm_text3') ?></textarea>
    <?php }
    function textHTML4() { ?>
        <textarea name="dowm_text4" id="desc_text4" cols="90" rows="5"><?php echo get_option('dowm_text4') ?></textarea>
    <?php }

    function adminPage() {
        add_options_page(
            'dowm Settings',
            'dowm',
            'manage_options',
            'dowm-settings',
            array($this, 'html')
        );
    }
    
    function html() {
        ?>
            <div class="wrap">
                <h1>dowm Settings</h1>
                <form action="options.php" method="POST">
                <?php
                    settings_fields('dowm');
                    do_settings_sections('dowm-settings');
                    submit_button();
                ?>
                </form>
                <p>NOTE: Use <code>[dowm_sc]</code> shortcode to add the button to the page/post.
            </div>
        <?php
    }
}

$dowmpluginAdmin = new DOWMPluginAdmin();



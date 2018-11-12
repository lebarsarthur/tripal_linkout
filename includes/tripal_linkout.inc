<?php

/**
 * Menu callbacks.
 */

/**
 * This version takes the gene "name", infers the species from the 5-letter prefix and delegates to the more fully specified version
 */
function gene_linkout_json($gene) {
    //find auxiliary info given the gene name
    //typically a polypeptide name (we stole this trick from the phylotree module)
    $sql = "SELECT f.feature_id as feature_id, f.uniquename AS gene_uniquename, o.genus AS genus, o.species AS species, o.organism_id as organism_id
            FROM chado.feature f, chado.organism o
            WHERE o.organism_id = f.organism_id
            AND f.name = :gene";

    $args = array(':gene' => $gene);
    $result = chado_query($sql, $args);
    $fields = $result->fetchAssoc();


    // Get mRNA from gene
    $sql = "SELECT f2.uniquename AS mrna_uniquename, f2.feature_id AS mrna_id
            FROM chado.feature f, chado.feature_relationship fr, chado.feature f2
            WHERE fr.type_id = (SELECT cvterm_id FROM chado.cvterm cvt, chado.cv cv
                              WHERE cvt.name = 'part_of' AND cv.name='sequence' AND cvt.cv_id = cv.cv_id)
            AND f.feature_id = fr.object_id
            AND fr.subject_id = f2.feature_id
            AND f.feature_id = :gene";

    $args = array(':gene' => $fields['feature_id']);
    $result = chado_query($sql, $args);

    $new_fields = $result->fetchAssoc();
    $fields['mrna_uniquename'] = $new_fields['mrna_uniquename'];
    $fields['mrna_id'] = $new_fields['mrna_id'];

    // Get polypeptide from mRNA
    $sql = "SELECT f2.uniquename AS pep_uniquename, f2.name AS pep_name
            FROM chado.feature f, chado.feature_relationship fr, chado.feature f2
            WHERE fr.type_id = (SELECT cvterm_id FROM chado.cvterm cvt, chado.cv cv
                              WHERE cvt.name = 'derives_from' AND cv.name='sequence' AND cvt.cv_id = cv.cv_id)
            AND f.feature_id = fr.object_id
            AND fr.subject_id = f2.feature_id
            AND f.feature_id = :gene";

    $args = array(':gene' => $fields['mrna_id']);
    $result = chado_query($sql, $args);

    $new_fields = $result->fetchAssoc();
    $fields['pep_uniquename'] = $new_fields['pep_uniquename'];
    $fields['pep_name'] = $new_fields['pep_name'];

    full_gene_linkout_json($fields['genus'], $fields['species'], $fields['pep_name'], $fields['pep_uniquename'], $fields['mrna_uniquename'], $gene, FALSE, $fields['organism_id']);
}

function family_representative_linkout_json($genus, $species, $family_representative) {
    //find the gene by use of the "family representative" (ie label used for the phylonode,
    //typically a polypeptide name (we stole this trick from the phylotree module)
    $sql = "SELECT f.name AS pep_name, f.uniquename AS pep_uniquename, f.feature_id AS pep_id, o.organism_id as organism_id
          FROM chado.feature f, chado.organism o
          WHERE o.organism_id = f.organism_id
          AND f.type_id = (SELECT cvterm_id FROM chado.cvterm
                              WHERE NAME = 'polypeptide')
          AND f.name = :family_representative";

    $args = array(':family_representative' => $family_representative);
    $result = chado_query($sql, $args);
    $fields = $result->fetchAssoc();

    // Get gene from mRNA
    $sql = "SELECT f2.uniquename AS gene_uniquename
          FROM chado.feature f, chado.feature_relationship fr, chado.feature f2
          WHERE fr.type_id = (SELECT cvterm_id FROM chado.cvterm cvt, chado.cv cv
                            WHERE cvt.name = 'part_of' AND cv.name='sequence' AND cvt.cv_id = cv.cv_id)
          AND f.feature_id = fr.subject_id
          AND fr.object_id = f2.feature_id
          AND f.name = :gene
          AND f.type_id in (SELECT cvterm_id FROM chado.cvterm
                              WHERE name = 'mRNA' OR name = 'transcript')";

    $args = array(':gene' => $family_representative);
    $result = chado_query($sql, $args);

    $new_fields = $result->fetchAssoc();
    $fields['gene_uniquename'] = $new_fields['gene_uniquename'];

    full_gene_linkout_json($genus, $species, $fields['pep_name'], $fields['pep_uniquename'], $family_representative, $fields['gene_uniquename'], TRUE, $fields['organism_id']);
}

function full_gene_linkout_json($genus, $species, $pep_name, $pep_uniquename, $transcript_name, $gene_name, $show_gcv=TRUE, $organism_id=NULL) {
    $link =  [];
    global $base_url;

    $current_gogepp = 'bipaa';
    if (strpos($base_url, 'bbip.genouest.org') !== false) {
        $current_gogepp = 'bbip';
    }

    $gogepp_url = 'http://'.$current_gogepp.'.genouest.org';

    $species = str_replace(' ', '-', $species);

    $sp_links = array();
    $sp_links['bombyx_mori'] = array('http://sgp.dna.affrc.go.jp/KAIKObase/keyword_search.php?keyword=%pep_uniquename%&field=all&chr_id=all&scaf_id=&data_set%5B%5D=gene&graph_view=on');
    $sp_links['helicoverpa_armigera'] = array('https://www.ncbi.nlm.nih.gov/protein/%pep_uniquename%/');
    $sp_links['heliconius_melpomene'] = array();
    $sp_links['danaus_plexippus'] = array();
    $sp_links['spodoptera_frugiperda-corn-v2.2'] = array($gogepp_url . '/apps/grs-2.3_sfru/grs?objectID=%pep_uniquename%&reportID=sfrudb_protein_report');
    $sp_links['spodoptera_frugiperda-rice-v2.3'] = array($gogepp_url . '/apps/grs-2.3_sfru/grs?reportID=sfrudb_rice_protein_report&objectID=%pep_uniquename%');
    $sp_links['spodoptera_frugiperda-corn-v5.0'] = array($gogepp_url . '/sp/spodoptera_frugiperda/feature/Spodoptera/frugiperda-corn/polypeptide/%pep_uniquename%');
    $sp_links['spodoptera_frugiperda-rice-v3.0'] = array($gogepp_url . '/sp/spodoptera_frugiperda/feature/Spodoptera/frugiperda-rice/polypeptide/%pep_uniquename%');
    $sp_links['aphis_glycines'] = array($gogepp_url . '/apps/grs-2.3_aglyc/grs?objectID=%pep_uniquename%&reportID=protein_report');
    $sp_links['myzus_persicae---clone-G006'] = array($gogepp_url . '/sp/myzus_persicae/feature/Myzus/persicae/polypeptide/%pep_uniquename%');
    $sp_links['myzus_persicae---clone-O'] = array($gogepp_url . '/sp/myzus_persicae/feature/Myzus/persicae/polypeptide/%pep_uniquename%');
    $sp_links['rhodnius_prolixus'] = array('http://metazoa.ensembl.org/Rhodnius_prolixus/Search/Results?species=Rhodnius%20prolixus;idx=;q=%pep_uniquename_noPA%');
    $sp_links['nilaparvata_lugens'] = array('https://www.ncbi.nlm.nih.gov/protein/%pep_uniquename%/');
    $sp_links['drosophila_melanogaster'] = array('https://www.ncbi.nlm.nih.gov/protein/%pep_uniquename%/');
    $sp_links['cimex_lectularius'] = array('https://www.ncbi.nlm.nih.gov/protein/%pep_uniquename%/');
    $sp_links['arabidopsis_thaliana'] = array('http://plants.ensembl.org/Arabidopsis_thaliana/Gene/Summary?db=core;g=%gene_name%');

    $org_str = strtolower($genus).'_'.$species;

    $pep_uniquename_noPA = explode('-P', $pep_uniquename)[0];

    $no_links = array(
        'diuraphis_noxia'
    );

    $replacements = array(
        '%genus%' => $genus,
        '%lower_genus%' => strtolower($genus),
        '%species%' => $species,
        '%pep_name%' => $pep_name,
        '%pep_uniquename%' => $pep_uniquename,
        '%pep_uniquename_noPA%' => $pep_uniquename_noPA,
        '%gene_name%' => $gene_name,
        '%transcript_name%' => $transcript_name,
    );

    if (array_key_exists($org_str, $sp_links)) {
        foreach ($sp_links[$org_str] as $key => $linkstr) {
            $link[] = array(
                'href' => str_replace(array_keys($replacements), $replacements, $linkstr),
                'text' => 'View page for: ' . $pep_uniquename,
            );
        }
    }
    else if (! in_array($org_str, $no_links)) {
        if (isset($pep_uniquename)) {
            $default_url = $gogepp_url . '/sp/%lower_genus%_%species%/feature/%genus%/%species%/polypeptide/%pep_uniquename%';
            $link[] = array(
                'href' => str_replace(array_keys($replacements), $replacements, $default_url),
                'text' => 'View page for: ' . $pep_uniquename,
            );
        }
    }

    if (!empty($gene_name) && $show_gcv) {
        $org = '';
        if (!empty($organism_id)) {
            $org = "/$organism_id";
        }
        if (substr($base_url, -6) === 'tripal')
            $default_url = $base_url . '/../gcv/#/search/'.$current_gogepp.$org.'/%gene_name%';
        else
            $default_url = $base_url . '/gcv/#/search/'.$current_gogepp.$org.'/%gene_name%';

        $link[] = array(
            'href' => str_replace(array_keys($replacements), $replacements, $default_url),
            'text' => 'View genomic context for: ' . $gene_name,
        );
    }

    drupal_json_output($link);
    drupal_exit();
}


/*
 * the api endpoint /famreps_links?famreps=feature1,feature2,etc...
 * is served by this function famreps_linkout_json().
 * accepts GET or POST.
 * Note: if using POST, use content-type:application/x-www-form-urlencoded
 * because PHP 5.6 will barf upon content-type:application/json.
 * see https://www.bram.us/2014/10/26/php-5-6-automatically-populating-http_raw_post_data-is-deprecated-and-will-be-removed-in-a-future-version/
 */
function famreps_linkout_json() {
    if($_SERVER['REQUEST_METHOD'] == 'POST') {
        // if content was posted sent with application/json, this would be best:
        // $encoded = file_get_contents("php://input", TRUE);
        // $q = drupal_json_decode($encoded, TRUE);
        // however in the workaround described above, we are forced
        // use x-www-form-urlencoded
        $encoded = filter_input(INPUT_POST, 'famreps', FILTER_SANITIZE_ENCODED);
        $q = explode('%2C', $encoded);
    }
    else {
        $encoded = filter_input(INPUT_GET, 'famreps', FILTER_SANITIZE_ENCODED);
        $q = explode('%2C', $encoded);
    }

    //find the gene by use of the "family representative" (ie label
    //used for the phylonode, typically a polypeptide name (we stole
    //this trick from the phylotree module)
    $sql = "SELECT f.name AS gene_name
          FROM chado.featureprop fp, chado.feature f
          WHERE fp.type_id = (SELECT cvterm_id FROM chado.cvterm
                              WHERE NAME = 'family representative')
          AND fp.value IN (:famreps)
          AND fp.feature_id = f.feature_id";

    // use chado_query(template, args) which 'will make sure that the
    // user input is santized and safe through type-checking and
    // escaping.'
    $args = array(':famreps' => $q);
    $result = chado_query($sql, $args);
    $genes = $result->fetchCol();
    $gene_list = implode(",", $genes);
    $link[] = array(
        'href' => '/lis_context_viewer/#/multi/'.$gene_list,
        'text' => 'View Multi-aligned Genome Contexts for given genes',
        'famreps' => implode(',', $q)
    );
    $gene_list = implode("%0A", $genes);
    $link[] = array(
        'href' => 'https://mines.legumeinfo.org/legumemine/bag.do?type=Gene&text='.$gene_list,
        'text' => 'Create List in LegumeMine for given Genes',
    );
    $link[] = array(
        'href' => 'https://mines.legumeinfo.org/legumemine/bag.do?type=Protein&text='.implode("%0A", $q),
        'text' => 'Create List in LegumeMine for given Proteins',
    );
    drupal_json_output(handle_relative_links($link));
    drupal_exit();
}

?>

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
        $sql = <<<SQL
          SELECT f.uniquename AS gene_uniquename, fp.value AS family_representative, o.genus AS genus, o.species AS species
          FROM chado.featureprop fp, chado.feature f, chado.organism o
          WHERE fp.type_id = (SELECT cvterm_id FROM chado.cvterm
                              WHERE NAME = 'family representative')
          AND fp.feature_id = f.feature_id
          AND o.organism_id = f.organism_id
          AND f.name = :gene
SQL;
        $args = array(':gene' => $gene);
        $result = chado_query($sql, $args);
        $fields = $result->fetchAssoc();
    full_gene_linkout_json($fields['genus'], $fields['species'], $gene, $fields['gene_uniquename'], $fields['family_representative']);
}

function family_representative_linkout_json($genus, $species, $family_representative) {
    //find the gene by use of the "family representative" (ie label used for the phylonode,
    //typically a polypeptide name (we stole this trick from the phylotree module)
    $sql = "SELECT f.name AS gene_name, f.uniquename AS gene_uniquename
          FROM chado.feature f
          WHERE f.uniquename = :family_representative";

    $args = array(':family_representative' => $family_representative);
    $result = chado_query($sql, $args);
    $fields = $result->fetchAssoc();
    full_gene_linkout_json($genus, $species, $fields['gene_name'], $fields['gene_uniquename'], $family_representative);
}

function full_gene_linkout_json($genus,$species,$gene_name,$gene_uniquename,$transcript_name) {
    $link =  [];

    $sp_split = explode(' ', $species);
    $species = $sp_split[0];

    $sp_links = array();
    $sp_links['bombyx_mori'] = array('http://example.org/%uniquename%');

    $org_str = strtolower($genus).'_'.$species;

    $replacements = array(
        '%genus%' => $genus,
        '%lower_genus%' => strtolower($genus),
        '%species%' => $species,
        '%name%' => $gene_name,
        '%uniquename%' => $gene_uniquename,
    );

    if (array_key_exists($org_str, $sp_links)) {

        foreach ($sp_links[$org_str] as $key => $linkstr) {
            $link[] = array(
                'href' => str_replace(array_keys($replacements), $replacements, $linkstr),
                'text' => 'View gene page for: ' . $gene_name,
            );
        }
    }
    else {
        if (isset($gene_uniquename)) {
            $default_url = 'http://bipaa.genouest.org/sp/%lower_genus%_%species%/feature/%genus%/%species%/polypeptide/%uniquename%';
            $link[] = array(
                'href' => str_replace(array_keys($replacements), $replacements, $default_url),
                'text' => 'View gene page for: ' . $gene_name,
            );
        }
        if (isset($gene_name)) {
            $default_url = '../../lis_context_viewer/#/search/lis/%uniquename%';
            $link[] = array(
                'href' => '../../gcv/#/search/lis/'.$gene_name,
                'text' => 'Find similar genomic contexts at LIS for : ' . $gene_name,
            );
        }
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
